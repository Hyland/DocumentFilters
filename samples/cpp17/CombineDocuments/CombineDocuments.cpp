/*
   (c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <DocumentFiltersObjects.h>
#include <DocumentFiltersSamples.h>
#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>

namespace DF = Hyland::DocFilters;

enum class ThumbnailOption
{
    None,
    First,
    All
};

struct options_t
{
    std::string license_key;

    std::string output_file_name = "output.pdf";
    bool recurse = false;
    ThumbnailOption thumbnail_page = ThumbnailOption::First;
    bool copy_bookmarks_from_source = false;
    std::vector<std::string> file_names;
};
static const std::map<std::string, ThumbnailOption> thumbnail_option_map{
	{"none", ThumbnailOption::None},
	{"first", ThumbnailOption::First},
	{"all", ThumbnailOption::All}
};

class program_t
{
public:
    DF::Api api;



    int OnExecute(const options_t& options)
    {
        api.Initialize(DocumentFiltersSamples::get_license_key(options.license_key), ".");

        auto files = enumerate_file_names(options.file_names, options.recurse);
        auto file_count = files.size();

		if (file_count == 0)
		{
			std::cerr << "No files found." << std::endl;
			return 1;
		}

        auto&& canvas = api.MakeOutputCanvas(DF::u8_to_w(options.output_file_name)
            , DocumentFiltersSamples::extension_to_canvas(std::filesystem::path(options.output_file_name).extension().string())
            , L"PDF_BOOKMARKS=on;HTML_INLINE_IMAGES=on");

        std::cerr << "Calculating Pages..." << std::endl;
        size_t thumbnails = 0;
        switch (options.thumbnail_page)
        {
        case ThumbnailOption::None:
            thumbnails = 0;
            break;
        case ThumbnailOption::First:
            thumbnails = file_count;
            break;
        case ThumbnailOption::All:
            thumbnails = count_all_pages(files);
            break;
        }

        size_t thumbnail_pages = static_cast<size_t>(std::ceil(thumbnails / static_cast<double>(m_thumbNailPagePerPage)));
        size_t prefix_pages = thumbnail_pages;

        if (thumbnail_pages > 0)
            make_thumbnail_pages(files, options, canvas, prefix_pages);

        std::vector<DF::Bookmark> replacement_bookmarks;

        size_t file_index = 0;
        for (const auto& file_name : files)
        {
            std::cerr << "Adding " << file_name << "..." << std::endl;
            try
            {
                auto file = api.OpenExtractor(file_name, DF::OpenMode::Paginated);
                auto page_count = file.getPageCount();

                std::cerr << "  Page 0 of " << page_count << "...\r";
                for (int page_index = 0; page_index < page_count; ++page_index)
                {
                    auto page = file.getPage(page_index);
                    canvas.RenderPage(page);

                    DF::AnnotationNamedDestination dest;
					dest.setName(L"D" + std::to_wstring(file_index + 1) + L"P" + std::to_wstring(page_index + 1));
					dest.setRect({ 0, 0, static_cast<int32_t>(page.getWidth()), static_cast<int32_t>(page.getHeight()) });

                    canvas.Annotate(dest);

                    std::cerr << "  Page " << page_index + 1 << " of " << page_count << "...\r";
                }
                std::cout << std::endl;

                DF::Bookmark file_bookmark;
                file_bookmark.setTitle(std::filesystem::path(file_name).filename().wstring());
                file_bookmark.setLevel(0);
				file_bookmark.setAction(DF::Bookmark::ActionType::GoTo);
                file_bookmark.setPageIndex(static_cast<uint32_t>(prefix_pages));

                replacement_bookmarks.push_back(copy_bookmarks(file_bookmark, file.getRootBookmark(), prefix_pages, 1));

                prefix_pages += file.getPageCount();
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Error: " << ex.what() << std::endl;
            }
            ++file_index;
        }

        if (options.copy_bookmarks_from_source)
        {
            canvas.ClearBookmarks();
            write_bookmarks(canvas, replacement_bookmarks.begin(), replacement_bookmarks.end());
        }

        canvas.Close();
        return 0;
    }

private:
    int m_thumbNailPageWidth = static_cast<int>(8.5 * 96);
    int m_thumbNailPageHeight = static_cast<int>(11 * 96);
    int m_thumbNailPageMargin = static_cast<int>(0.5 * 96);
    int m_thumbNailPageAvailableWidth = m_thumbNailPageWidth - (m_thumbNailPageMargin * 2);
    int m_thumbNailPageAvailableHeight = m_thumbNailPageHeight - (m_thumbNailPageMargin * 2);
    int m_thumbNailPageAcross = 4;
    int m_thumbNailPageDown = 5;
    int m_thumbNailHorzSpace = 5;
    int m_thumbNailVertSpace = 5;
    int m_thumbNailPagePerPage = m_thumbNailPageAcross * m_thumbNailPageDown;
    int m_thumbNailWidth = m_thumbNailPageAvailableWidth / m_thumbNailPageAcross;
    int m_thumbNailHeight = m_thumbNailPageAvailableHeight / m_thumbNailPageDown;

    struct page_info_t
    {
        std::string filename;
        DF::Extractor extractor;
        size_t extractor_index;
        DF::Page page;
        size_t page_index;
    };


    std::vector<std::string> enumerate_file_names(const std::vector<std::string>& files_or_directories, bool recurse)
    {
		auto matches = [](const std::string& name, const std::string& pattern) -> bool
			{
                if (pattern.empty() || pattern == "*")
                    return true;

                if (pattern.find("*.") == 0)
                    return name.find(pattern.substr(1)) != name.size() - pattern.size() - 1;
                   
                return name.find(pattern) != std::string::npos;
			};

        std::vector<std::string> result;
        for (const auto& file_or_directory : files_or_directories)
        {
            std::string item = file_or_directory;
            std::string pattern = std::filesystem::path(file_or_directory).filename().string();
            if (pattern.find_first_of("*?") == std::string::npos)
                pattern = "*";
            else
                item = std::filesystem::path(file_or_directory).parent_path().string();

            if (std::filesystem::is_directory(item) && recurse)
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(item))
                {
                    if (entry.is_regular_file() && matches(std::filesystem::path(entry.path()).filename().string(), pattern))
                        result.push_back(entry.path().string());
                }
            }
            else if (std::filesystem::is_regular_file(item))
                result.push_back(item);
        }
        return result;
    }

    void enumerate_pages(const std::vector<std::string>& file_names, ThumbnailOption opt, const std::function<bool(const page_info_t& info)>& callback)
    {
        std::wstring options;
        if (opt == ThumbnailOption::First)
            options = L"LIMIT_PAGES=1";

        size_t file_index = 0;
        for (auto&& file : file_names)
        {
            try
            {
                page_info_t info;
				info.filename = file;
				info.extractor_index = file_index++;
                info.extractor = api.OpenExtractor(DF::u8_to_w(file), DF::OpenMode::Paginated, 0, options);

				size_t page_count = info.extractor.getPageCount();
				if (opt == ThumbnailOption::First)
					page_count = 1;

				for (size_t page_index = 0; page_index < page_count; ++page_index)
				{
                    info.page_index = page_index;
                    info.page = info.extractor.getPage(page_index);
					if (!callback(info))
						return;
				}
			}
			catch (const std::exception& ex) {
                std::cerr << std::endl << "Error Processing " << file << std::endl
                    << "  - " << ex.what() << std::endl;
			}
        }
    }

    size_t count_all_pages(const std::vector<std::string>& file_names)
    {
        size_t result = 0;
        enumerate_pages(file_names, ThumbnailOption::First, [&](const page_info_t& info) {
			result += info.extractor.getPageCount();
            return true;
            });

        return result;
    }

    DF::Bookmark copy_bookmarks(DF::Bookmark dest, DF::Bookmark source, size_t adjust, size_t level = 0)
    {
        for (const auto& child : source.children())
        {
            DF::Bookmark new_child;
            new_child.setTitle(child.getTitle());
            new_child.setAction(child.getAction());
            new_child.setPageIndex(static_cast<uint32_t>(child.getPageIndex() + adjust));
            new_child.setLevel(static_cast<uint32_t>(level));
            dest.AppendChild(new_child);
            copy_bookmarks(new_child, child, adjust, level + 1);
        }
        return dest;
    }

    template <typename Iter>
    void write_bookmarks(DF::Canvas& canvas, Iter begin, Iter end)
    {
		for (; begin != end; ++begin)
		{
			canvas.AppendBookmark(*begin);
			write_bookmarks(canvas, begin->children().begin(), begin->children().end());
		}
    }

    void make_thumbnail_pages(const std::vector<std::string>& files, const options_t& options, DF::Canvas& canvas, size_t prefix_pages)
    {
        int x = m_thumbNailPageMargin;
        int y = m_thumbNailPageMargin;
        int caption_height = 20;
        size_t page_index = 0;

        std::cerr << "Generating thumbnail 0 of " + std::to_string(files.size()) + "...";

		enumerate_pages(files, options.thumbnail_page, [&](const page_info_t& info) -> bool
			{
                auto&& doc = info.extractor;
				auto&& page = info.page;

                std::cerr << "\rGenerating thumbnail " + std::to_string(info.extractor_index + 1) + " of " + std::to_string(files.size()) + "...";

                if (page_index == 0 || page_index % m_thumbNailPagePerPage == 0)
                {
                    canvas.BlankPage(m_thumbNailPageWidth, m_thumbNailPageHeight);

                    x = m_thumbNailPageMargin;
                    y = m_thumbNailPageMargin;

                    canvas.SetFont(L"Arial", 9);
                    caption_height = static_cast<int>(canvas.TextHeight(L"aGj") * 1.2);
                }
                else if (page_index % m_thumbNailPageAcross == 0)
                {
                    x = m_thumbNailPageMargin;
                    y += m_thumbNailHeight;
                }

                double page_aspect_ratio = static_cast<double>(page.getHeight()) / page.getWidth();
                int available_height = m_thumbNailHeight - caption_height - (m_thumbNailVertSpace * 2);
                int available_width = m_thumbNailWidth - (m_thumbNailHorzSpace * 2);

                DF::VectorStream thumbnail_stream;
                
                auto thumbnail_canvas = api.MakeOutputCanvas(thumbnail_stream, DF::CanvasType::PNG);
                thumbnail_canvas.RenderPage(page, L"GRAPHIC_WIDTH=" + std::to_wstring(available_width));
				thumbnail_canvas.Close();

                auto tile_rect = DF::Rect::ltrb(
                    x + m_thumbNailHorzSpace,
                    y + m_thumbNailVertSpace,
                    x + m_thumbNailWidth - m_thumbNailHorzSpace,
                    y + m_thumbNailHeight - m_thumbNailVertSpace
                );

                auto thumb_space_rect = DF::Rect::ltrb(
                    tile_rect.left + m_thumbNailHorzSpace,
                    tile_rect.top + m_thumbNailVertSpace,
                    tile_rect.right - m_thumbNailHorzSpace,
                    tile_rect.bottom - caption_height - m_thumbNailHorzSpace
                );

                auto thumb_rect = DF::Rect::xywh(0, 0, thumb_space_rect.width(), static_cast<int>(thumb_space_rect.width() * page_aspect_ratio));
                if (thumb_rect.height() > thumb_space_rect.height())
                    thumb_rect = DF::Rect::xywh(0, 0, static_cast<int>(thumb_space_rect.height() / page_aspect_ratio), thumb_space_rect.height());

                thumb_rect.offset(thumb_space_rect.x() + (thumb_space_rect.width() - thumb_rect.width()) / 2
                    , thumb_space_rect.y() + (thumb_space_rect.height() - thumb_rect.height()) / 2);

                canvas.SetPen(DF::Color(0x9f, 0x9f, 0x9f), 1, DF::PenStyle::Solid);
                canvas.SetBrush(DF::Color(0xef, 0xef, 0xef), DF::BrushStyle::Solid);
                canvas.Rect(tile_rect);
                canvas.DrawScaleImage(thumb_rect.left
                    , thumb_rect.top
                    , thumb_rect.right
                    , thumb_rect.bottom
                    , &thumbnail_stream.data()[0]
                    , thumbnail_stream.size()
                    , L"image/png");

                canvas.SetBrush(DF::Color::black());
                canvas.SetFont(L"Arial", 9);
                std::wstring caption = info.page_index == 0 ? DF::u8_to_w(info.filename) : L"Page " + std::to_wstring(info.page_index + 1);
                std::wstring caption_suffix;
                int caption_width = canvas.TextWidth(caption);
                while (caption_width > tile_rect.width() && !caption.empty())
                {
                    caption.pop_back();
                    caption_suffix = L"...";
                    caption_width = canvas.TextWidth(caption + caption_suffix);
                }
                canvas.TextOut(tile_rect.left + (tile_rect.width() - caption_width) / 2, tile_rect.bottom - caption_height, caption + caption_suffix);

                DF::AnnotationLink link;
                link.setRect(tile_rect);
                link.getAction().setType(DF::AnnotationAction::ActionType::GoTo);
                link.getAction().setName(L"D" + std::to_wstring(info.extractor_index + 1) + L"P" + std::to_wstring(info.page_index + 1));
                canvas.Annotate(link);

                x += m_thumbNailWidth;
                page_index++;
				return true;
			});

        std::cerr << std::endl;
    }
};

int main(int argc, char* argv[])
{
    CLI::App app("Hyland Document Filters: ConvertDocumentToClassicHTML");

    try {
        options_t options;

        app.add_option("-l,--license", options.license_key, "license key for Document Filters");
        app.add_option("-o,--output", options.output_file_name, "Output file name");
        app.add_flag("-r,--recurse", options.recurse, "Walk into sub directories");
        app.add_option("-t,--thumbnails", options.thumbnail_page, "Thumbnail page option")
            ->transform(CLI::CheckedTransformer(thumbnail_option_map, CLI::ignore_case));
        app.add_flag("--copy-bookmarks", options.copy_bookmarks_from_source, "Copy bookmarks from source documents");
        app.add_option("filename", options.file_names, "Files to process")->required();
        app.parse(argc, argv);

		program_t program;
		return program.OnExecute(options);
    }
    catch (const CLI::ParseError& e) {
        return app.exit(e);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
