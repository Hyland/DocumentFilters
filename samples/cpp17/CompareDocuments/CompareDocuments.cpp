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
#include <iostream>
#include <string>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cmath>


namespace DF = Hyland::DocFilters;

struct options_t
{
    std::string license_key;
    std::string output_file_name = "output.pdf";
    bool ignore_case = false;
    bool ignore_fields = false;
    bool ignore_footers = false;
    bool ignore_headers = false;
    bool ignore_punctuation = false;
    bool ignore_tables = false;
    bool ignore_text_boxes = false;
    bool include_formatting = false;
    std::string left_options;
    std::string left_output;
    std::string left_top_margin;
    std::string left_bottom_margin;
    std::string options;
    std::string pages;
    std::string right_options;
    std::string right_output;
    std::string right_top_margin;
    std::string right_bottom_margin;
    bool silent = false;
    std::string left_file;
    std::string right_file;

};

std::pair<int, int> parsePages(const std::string& pages) {
    if (pages.empty())
        return { 0, 0xffff };

    auto pos = pages.find('-');
    if (pos == std::string::npos) {
        int page;
        std::istringstream(pages) >> page;
        return { page, page };
    }

    int first, last;
    std::istringstream(pages.substr(0, pos)) >> first;
    std::istringstream(pages.substr(pos + 1)) >> last;

    if (first == 0)
        throw std::invalid_argument("First page must be an integer");

    if (last == 0)
        last = 0xffff;

    return { first, last };
}

bool ends_with(const std::string& haystack, const std::string& needle) {
	if (haystack.size() < needle.size())
		return false;
	return haystack.compare(haystack.size() - needle.size(), needle.size(), needle) == 0;
}

float unpackMeasurement(const std::string& measurement) {
    if (measurement.empty())
        return 0;

    float value = std::stof(measurement.substr(0, measurement.size() - 2));
    if (ends_with(measurement, "in"))
        return value * 72;
    if (ends_with(measurement, "cm"))
        return value * 28.35f;
    if (ends_with(measurement, "mm"))
        return value * 2.835f;
    if (ends_with(measurement, "pt"))
        return value;
    if (ends_with(measurement, "px"))
        return value * 96 / 72;

    return std::stof(measurement);
}

void ensurePage(int pageIndex, DF::Extractor& doc, DF::Canvas& canvas, int& currentPage, bool isLeft) {
    while (currentPage < pageIndex) {
        auto page = doc.getPage(++currentPage);
        canvas.RenderPage(page);
    }
    canvas.SetBrush(isLeft ? DF::Color(0xff, 0x00, 0x00, 0x50) : DF::Color(0x00, 0xff, 0x00, 0x50));
    canvas.SetPen(DF::Color::transparent(), 0, DF::PenStyle::None);
}

int main(int argc, char* argv[])
{
    CLI::App app("Hyland Document Filters: ConvertDocumentToClassicHTML");

    try {
        options_t options;

        app.add_option("-l,--license", options.license_key, "license key for Document Filters");
        app.add_option("-o,--output", options.output_file_name, "Output file name");
        app.add_flag("--ignore-case", options.ignore_case, "Case insensitive comparison");
        app.add_flag("--ignore-fields", options.ignore_fields, "Exclude fields from comparison");
        app.add_flag("--ignore-footers", options.ignore_footers, "Exclude marked footers from comparison");
        app.add_flag("--ignore-headers", options.ignore_headers, "Exclude marked headers from comparison");
        app.add_flag("--ignore-punctuation", options.ignore_punctuation, "Exclude punctuation from comparison");
        app.add_flag("--ignore-tables", options.ignore_tables, "Exclude tables from comparison");
        app.add_flag("--ignore-textboxes", options.ignore_text_boxes, "Exclude textboxes from comparison");
        app.add_flag("--include-formatting", options.include_formatting, "Include formatting in comparison");
        app.add_option("--left-options", options.left_options, "Option string that applies to the first document");
        app.add_option("--left-output", options.left_output, "Filename where marked up left output should be saved");
        app.add_option("--left-top-margin", options.left_top_margin, "The size of the top margin that should be excluded");
        app.add_option("--left-bottom-margin", options.left_bottom_margin, "The size of the bottom margin that should be excluded");
        app.add_option("--options", options.options, "Option string that applies to both documents");
        app.add_option("--pages", options.pages, "Pages to compare in the form of FIRST-LAST");
        app.add_option("--right-options", options.right_options, "Option string that applies to the second document");
        app.add_option("--right-output", options.right_output, "Filename where marked up right output should be saved");
        app.add_option("--right-top-margin", options.right_top_margin, "The size of the top margin that should be excluded");
        app.add_option("--right-bottom-margin", options.right_bottom_margin, "The size of the bottom margin that should be excluded");
        app.add_flag("--silent", options.silent, "Disable writing to console");
        app.add_option("left_file", options.left_file, "Left file")->required();
        app.add_option("right_file", options.right_file, "Right file")->required();
        app.parse(argc, argv);

        if (options.left_file.empty())
			throw std::runtime_error("Left file is required");
		if (options.right_file.empty())
			throw std::runtime_error("Right file is required");

		DF::Api api(DocumentFiltersSamples::get_license_key(options.license_key), ".");
        DF::CompareSettings compareSettings;
		compareSettings.setType(DF::CompareType::Word);

        // Set compare settings flags based on options
        if (options.ignore_case)
            compareSettings.setFlag(DF::CompareFlags::NoCase);
        if (options.ignore_fields)
            compareSettings.setFlag(DF::CompareFlags::NoFields);
        if (options.ignore_footers)
            compareSettings.setFlag(DF::CompareFlags::NoFooters);
        if (options.ignore_headers)
            compareSettings.setFlag(DF::CompareFlags::NoHeaders);
        if (options.ignore_punctuation)
            compareSettings.setFlag(DF::CompareFlags::NoPunctuation);
        if (options.ignore_tables)
            compareSettings.setFlag(DF::CompareFlags::NoTables);
        if (options.ignore_text_boxes)
            compareSettings.setFlag(DF::CompareFlags::NoTextBoxes);
        if (options.include_formatting)
            compareSettings.setFlag(DF::CompareFlags::Formatting);

        // Parse pages
        auto pages = parsePages(options.pages);

        // Set up left page settings
        DF::CompareDocumentSettings leftPageSettings;
        leftPageSettings.setFirstPage(pages.first);
        leftPageSettings.setPageCount(pages.second - pages.first + 1);
        leftPageSettings.setMargins(DF::RectF::ltrb(0, unpackMeasurement(options.left_top_margin), 0, unpackMeasurement(options.left_bottom_margin)));

        // Set up right page settings
        DF::CompareDocumentSettings rightPageSettings;
        rightPageSettings.setFirstPage(pages.first);
        rightPageSettings.setPageCount(pages.second - pages.first + 1);
        rightPageSettings.setMargins(DF::RectF::ltrb(0, unpackMeasurement(options.right_top_margin), 0, unpackMeasurement(options.right_bottom_margin)));

        // Open extractors
        auto left = api.OpenExtractor(DF::u8_to_w(options.left_file), DF::OpenMode::Paginated, IGR_BODY_AND_META, DF::u8_to_w(options.options + ";" + options.left_options));
        auto right = api.OpenExtractor(DF::u8_to_w(options.right_file), DF::OpenMode::Paginated, IGR_BODY_AND_META, DF::u8_to_w(options.options + ";" + options.right_options));

        // Perform comparison
        auto compareResult = left.Compare(leftPageSettings, right, rightPageSettings, compareSettings);

        // Create output canvases if specified
        DF::Canvas leftOutputCanvas;
        if (!options.left_output.empty()) {
            leftOutputCanvas = api.MakeOutputCanvas(options.left_output, DF::CanvasType::PDF);
        }

        DF::Canvas rightOutputCanvas;
        if (!options.right_output.empty()) {
            rightOutputCanvas = api.MakeOutputCanvas(options.right_output, DF::CanvasType::PDF);
        }

        int currentLeft = -1, currentRight = -1;
        while (compareResult.MoveNext())
        {
			auto&& current = compareResult.getCurrent();
            if (current.getType() == DF::DifferenceType::NextBatch)
                continue;

            for (const auto& hit : current.details())
            {
                if (current.getType() == DF::DifferenceType::Delete && leftOutputCanvas.hasHandle()) {
                    ensurePage(hit.getPageIndex(), left, leftOutputCanvas, currentLeft, true);
                    leftOutputCanvas.Rect(hit.getBounds());
                }

                if (current.getType() == DF::DifferenceType::Insert && rightOutputCanvas.hasHandle()) {
                    ensurePage(hit.getPageIndex(), right, rightOutputCanvas, currentRight, false);
                    rightOutputCanvas.Rect(hit.getBounds());
                }
            }
        }


    }
    catch (const CLI::ParseError& e) {
        return app.exit(e);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
