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
using Hyland.DocumentFilters;
using McMaster.Extensions.CommandLineUtils;

class Program
{
    public enum ThumbnailOption
    {
        None,
        First,
        All
    }

    [Option("-o|--output", Description = "Output file name")]
    public string OutputFileName { get; set; } = "output.pdf";

    [Option("-r|--recurse", "Walk into sub directories", CommandOptionType.NoValue)]
    public bool Recurse { get; set; }

    [Option("-t|--thumbnails", Description = "Thumbnail page option")]
    public ThumbnailOption ThumbnailPage { get; set; } = ThumbnailOption.First;

    [Option("--copy-bookmarks", "Copy bookmarks from source documents", CommandOptionType.NoValue)]
    public bool CopyBookmarksFromSource { get; set; }

    [Argument(0, Description = "Files to process")]
    public List<string> FileNames { get; set; } = new();

    private readonly Hyland.DocumentFilters.Api _api = new();

    private readonly int _thumbNailPageWidth = (int)(8.5 * 96);
    private readonly int _thumbNailPageHeight = (int)(11 * 96);
    private readonly int _thumbNailPageMargin = (int)(0.5 * 96);
    private int _thumbNailPageAvailableWidth => _thumbNailPageWidth - (_thumbNailPageMargin * 2);
    private int _thumbNailPageAvailableHeight => _thumbNailPageHeight - (_thumbNailPageMargin * 2);
    private readonly int _thumbNailPageAcross = 4;
    private readonly int _thumbNailPageDown = 5;
    private readonly int _thumbNailHorzSpace = 5;
    private readonly int _thumbNailVertSpace = 5;
    private int _thumbNailPagePerPage => _thumbNailPageAcross * _thumbNailPageDown;
    private int _thumbNailWidth => (int)(_thumbNailPageAvailableWidth / _thumbNailPageAcross);
    private int _thumbNailHeight => (int)(_thumbNailPageAvailableHeight / _thumbNailPageDown);

    public int OnExecute()
    {
        _api.Initialize(DocumentFiltersLicense.Get(), ".");

        IEnumerable<string> files = EnumerateFileNames(FileNames, Recurse);
        int fileCount = files.Count();

        using (Canvas canvas = _api.MakeOutputCanvas(OutputFileName, ExtToCanvasType(Path.GetExtension(OutputFileName)), "PDF_BOOKMARKS=on;HTML_INLINE_IMAGES=on"))
        {
            Console.Error.WriteLine("Calculating Pages...");
            int thumbnails = ThumbnailPage switch
            {
                ThumbnailOption.None => 0,
                ThumbnailOption.First => fileCount,
                ThumbnailOption.All => EnumerateAllPages(files).Count(),
                _ => 0
            };
            int thumbnailPages = (int)Math.Ceiling(thumbnails / (double)_thumbNailPagePerPage);
            int prefixPages = thumbnailPages;

            if (thumbnailPages > 0)
            {
                Console.Error.WriteLine("Generating Thumbnails...");
                MakeThumbnailPages(canvas, EnumeratePages(files, ThumbnailPage), prefixPages);
            }

            List<Bookmark> replacementBookmarks = new();

            foreach ((string fileName, int fileIndex) in files.Select((file, fileIndex) => (file, fileIndex)))
            {
                Console.Error.WriteLine($"Adding {fileName}...");
                try
                {
                    using Extractor file = _api.OpenExtractor(fileName, OpenMode.Paginated);
                    int pageCount = file.PageCount;

                    Console.Error.Write($"  Page {0} of {pageCount}...\r");
                    foreach ((Page page, int pageIndex) in file.Pages.Select((page, pageIndex) => (page, pageIndex)))
                    {
                        using (page)
                        {
                            canvas.RenderPage(page);

                            canvas.Annotate(new Hyland.DocumentFilters.Annotations.NamedDestination
                            {
                                Name = $"D{fileIndex + 1}P{pageIndex + 1}",

                                Rect = System.Drawing.Rectangle.FromLTRB(0, 0, page.Width, page.Height),
                            });
                        }

                        Console.Error.Write($"  Page {pageIndex + 1} of {pageCount}...\r");
                    }
                    Console.WriteLine("");

                    // Rebuild the bookmarks adjusting for pages that have been added
                    Bookmark fileBookmark = new()
                    {
                        Title = Path.GetFileName(fileName),
                        Level = 0,
                        Action = Bookmark.ActionType.GoTo,
                        PageIndex = prefixPages
                    };

                    replacementBookmarks.Add(CopyBookmarks(fileBookmark, file.RootBookmark, prefixPages, 1));

                    prefixPages += file.PageCount;

                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"Error: {ex.Message}");
                }
            }

            if (CopyBookmarksFromSource)
            {
                canvas.ClearBookmarks();
                WriteBookmarks(canvas, replacementBookmarks);
            }

            canvas.Close();
        }

        return 0;
    }

    private class PageInfo
    {
        public string Filename { get; set; } = "";
        public Extractor? Extractor { get; set; }
        public int ExtractorIndex { get; set; }

        public Page? Page { get; set; }
        public int PageIndex { get; set; }
    }

    private IEnumerable<PageInfo> EnumeratePages(IEnumerable<string> fileNames, Func<int, bool> func)
    {
        int fileIndex = 0;
        foreach (string fileName in fileNames)
        {
            using Extractor? file = GetOrNull(() => _api.OpenExtractor(fileName, OpenMode.Paginated));
            if (file != null)
            {
                PageInfo info = new();
                info.Filename = Path.GetFileName(fileName);
                info.Extractor = file;
                info.ExtractorIndex = fileIndex;
                for (int i = 0, c = file.Pages.Count; i < c; i++)
                {
                    if (func(i))
                    {
                        info.Page = file.GetPage(i);
                        info.PageIndex = i;
                        yield return info;
                    }
                    else
                        break;
                }
            }

            ++fileIndex;
        }
    }

    private IEnumerable<PageInfo> EnumerateAllPages(IEnumerable<string> fileNames) => EnumeratePages(fileNames, (i) => true);

    private IEnumerable<PageInfo> EnumeratePages(IEnumerable<string> fileNames, ThumbnailOption opt)
    {
        Func<int, bool> func = opt switch
        {
            ThumbnailOption.None => (int i) => false,
            ThumbnailOption.All => (int i) => true,
            ThumbnailOption.First => (int i) => i == 0,
            _ => (int i) => false
        };

        return EnumeratePages(fileNames, func);
    }

    private T? GetOrNull<T>(Func<T> action)
    {
        try
        {
            return action();
        }
        catch (Exception)
        {
            return default;
        }
    }

    private Bookmark CopyBookmarks(Bookmark dest, Bookmark source, int adjust, int level = 0)
    {
        foreach (Bookmark? child in source.Children)
        {
            Bookmark newChild = new()
            {
                Title = child.Title,
                Action = child.Action,
                PageIndex = child.PageIndex + adjust,
                Level = level
            };
            dest.AddChild(newChild);
            _ = CopyBookmarks(newChild, child, adjust, level + 1);
        }
        return dest;
    }

    private void WriteBookmarks(Canvas canvas, IEnumerable<Bookmark> root)
    {
        foreach (Bookmark child in root)
        {
            canvas.AppendBookmark(child);
            WriteBookmarks(canvas, child.Children);
        }
    }

    private void MakeThumbnailPages(Canvas canvas, IEnumerable<PageInfo> pages, int prefixPages)
    {
        int x = _thumbNailPageMargin;
        int y = _thumbNailPageMargin;
        int captionHeight = 20;

        foreach ((PageInfo page, int pageIndex) in pages.Select((page, pageIndex) => (page, pageIndex)))
        {
            using (page.Page)
            {
                if (pageIndex == 0 || pageIndex % _thumbNailPagePerPage == 0)
                {
                    canvas.BlankPage(_thumbNailPageWidth, _thumbNailPageHeight);

                    x = _thumbNailPageMargin;
                    y = _thumbNailPageMargin;

                    canvas.SetFont("Arial", 9, 0);
                    captionHeight = (int)(canvas.TextHeight("aGj") * 1.2);
                }
                else if (pageIndex % _thumbNailPageAcross == 0)
                {
                    x = _thumbNailPageMargin;
                    y += _thumbNailHeight;
                }

                double pageAspectRatio = (double)page.Page!.Height / page.Page.Width;
                int availableHeight = _thumbNailHeight - captionHeight - (_thumbNailVertSpace * 2);
                int availableWidth = _thumbNailWidth - (_thumbNailHorzSpace * 2);

                // Generate a thumbnail image of the page.
                using MemoryStream thumbnailStream = new();
                {
                    using Canvas thumbnailCanvas = _api.MakeOutputCanvas(thumbnailStream, CanvasType.PNG);
                    thumbnailCanvas.RenderPage(page.Page, $"GRAPHIC_WIDTH={availableWidth}");
                }

                // Calculate the size in the destination
                System.Drawing.Rectangle tileRect = System.Drawing.Rectangle.FromLTRB(
                      x + _thumbNailHorzSpace
                    , y + _thumbNailVertSpace
                    , x + _thumbNailWidth - _thumbNailHorzSpace
                    , y + _thumbNailHeight - _thumbNailVertSpace);
                System.Drawing.Rectangle thumbSpaceRect = System.Drawing.Rectangle.FromLTRB(
                    tileRect.Left + _thumbNailHorzSpace,
                    tileRect.Top + _thumbNailVertSpace,
                    tileRect.Right - _thumbNailHorzSpace,
                    tileRect.Bottom - captionHeight - _thumbNailHorzSpace);

                System.Drawing.Rectangle thumbRect = new(0, 0, thumbSpaceRect.Width, (int)(thumbSpaceRect.Width * pageAspectRatio));
                if (thumbRect.Height > thumbSpaceRect.Height)
                    thumbRect = new System.Drawing.Rectangle(0, 0, (int)(thumbSpaceRect.Height / pageAspectRatio), thumbSpaceRect.Height);

                // Center the thumb in the dest rect
                thumbRect.Offset(thumbSpaceRect.X + (thumbSpaceRect.Width - thumbRect.Width) / 2
                    , thumbSpaceRect.Y + (thumbSpaceRect.Height - thumbRect.Height) / 2);

                canvas.SetPen(0x9f9f9f, 1, 1);
                canvas.SetBrush(0xefefef, 1);
                canvas.Rect(tileRect);
                canvas.DrawScaleImage(thumbRect.X, thumbRect.Y, thumbRect.Right, thumbRect.Bottom, thumbnailStream.ToArray(), "image/png");

                // Caption
                canvas.SetBrush(0, 1); // solid black brush
                canvas.SetFont("Arial", 9, 0);
                string caption = page.PageIndex == 0 ? page.Filename : $"Page {page.PageIndex + 1}";
                string captionSuffix = "";
                int captionWidth = canvas.TextWidth(caption);
                while (captionWidth > tileRect.Width && caption.Length > 0)
                {
                    caption = caption.Substring(0, caption.Length - 1);
                    captionSuffix = "...";
                    captionWidth = canvas.TextWidth(caption + captionSuffix);
                }
                canvas.TextOut(tileRect.X + (tileRect.Width - captionWidth) / 2, tileRect.Bottom - captionHeight, caption + captionSuffix);

                canvas.Annotate(new Hyland.DocumentFilters.Annotations.Link
                {
                    Rect = tileRect,
                    Action = new Hyland.DocumentFilters.Annotations.ActionGoTo
                    {
                        Name = $"D{page.ExtractorIndex + 1}P{page.PageIndex + 1}"
                    }
                });

                x += _thumbNailWidth;
            }
        }
    }

    public static IEnumerable<string> EnumerateFileNames(IEnumerable<string> filesOrDirectories, bool recurse)
    {

        foreach (string fileOrDirectory in filesOrDirectories)
        {
            string? item = fileOrDirectory;
            string pattern = Path.GetFileName(fileOrDirectory);
            if (pattern.IndexOfAny(new char[] { '*', '?' }) < 0)
                pattern = "*";
            else
                item = Path.GetDirectoryName(fileOrDirectory);

            if (System.IO.Directory.Exists(item))
            {
                string[] children = System.IO.Directory.GetFiles(item, pattern, recurse
                    ? System.IO.SearchOption.AllDirectories
                    : System.IO.SearchOption.TopDirectoryOnly);
                foreach (string child in children)
                    yield return child;
            }
            else if (System.IO.File.Exists(item))
                yield return item;
        }
    }

    private CanvasType ExtToCanvasType(string? ext)
    {
        CanvasType res;
        if (!Enum.TryParse<CanvasType>(ext?.Trim('.'), true, out res))
            throw new ArgumentException($"Unknown format {ext}");
        return res;
    }

    static int Main(string[] args) => CommandLineApplication.Execute<Program>(args);
}

