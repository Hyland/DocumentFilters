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
using System.Linq;
using System.Runtime.InteropServices;


class Program
{
    [Argument(0, Description = "Input filename")]
    public string? FileName { get; set; }

    [Option("-o|--output", Description = "Output filename")]
    public string? OutputFilename { get; set; } = "output.pdf";

    [Option("-c|--comments", Description = "Where to render comments")]
    public CommentsLocation CommentsLocation { get; set; } = CommentsLocation.Margin;

    [Option("--comment-overflow", Description = "Controls what happens to comments that do not fit in the margin")]
    public CommentOverflow CommentOverflow { get; set; } = CommentOverflow.NewPage;

    [Option("-m|--margin-size", Description = "Size of comments margin in pixels")]
    public int MarginSize { get; set; } = 250;

    [Option("--margin-color", Description = "The color of the comments panel")]
    public string MarginColor { get; set; } = "#fefefe";

    [Option("--margin-border", Description = "The color of the comments panel border")]
    public string MarginBorderColor { get; set; } = "#e0e0e0";

    [Option("--margin-hide", Description = "Hide the comment margin when there are no comments")]
    public bool HideWhenNoComments { get; set; } = false;


    private readonly Hyland.DocumentFilters.Api _api = new();

    public int OnExecute()
    {
        _api.Initialize(DocumentFiltersLicense.Get(), ".");

        using var doc = _api.OpenExtractor(FileName, Hyland.DocumentFilters.OpenMode.Paginated);
        using var canvas = _api.MakeOutputCanvas(OutputFilename, ExtToCanvasType(Path.GetExtension(OutputFilename)));

        System.Drawing.Color commentsBg = System.Drawing.ColorTranslator.FromHtml(MarginColor);
        System.Drawing.Color commentsBorder = System.Drawing.ColorTranslator.FromHtml(MarginBorderColor);
        List<Comment> endOfDocComments = new();

        foreach (var page in doc.Pages.Select((item, index) => new { item, index }))
        {
            using (page.item)
            {
                List<Comment>? marginComments = new();
                List<Comment>? endOfPageComments = new();

                var commentPanel = CommentsLocation switch
                {
                    CommentsLocation.Margin => (Width: MarginSize, Height: page.item.Height),
                    CommentsLocation.AfterPage => (Width: page.item.Width, Height: page.item.Height),
                    CommentsLocation.AfterDocument => (Width: (int)8.5 * 96, Height: (int)11 * 96),
                    _ => (Width: 0, Height: 0)
                };

                // Collect the comments on this page
                var commentAnnos = page.item.Annotations.Where(x => (x.Flags & Hyland.DocumentFilters.Annotations.Flags.Hidden) == 0 && !string.IsNullOrEmpty(x.Text))
                    .Select(anno => new Comment(anno) { Location = $"Page {page.index + 1}" })
                    .OrderBy(x => x.Annotation.Rect.Bottom).ToList();
                foreach (var c in commentAnnos)
                {
                    endOfPageComments.Add(c);
                    // Add comment replies
                    endOfPageComments.AddRange(c.Annotation.Replies.Where(x => (x.Flags & Hyland.DocumentFilters.Annotations.Flags.Hidden) == 0 && !string.IsNullOrEmpty(x.Text))
                        .Select(repl => new Comment(repl, true)));
                }

                // Draw the page content
                if (CommentsLocation == CommentsLocation.Margin && (!HideWhenNoComments || endOfPageComments.Count > 0))
                {
                    canvas.BlankPage((int)(page.item.Width + MarginSize), page.item.Height);

                    using var pageCanvasStream = new MemoryStream();
                    {
                        using var pageCanvas = _api.MakeOutputCanvas(pageCanvasStream, Hyland.DocumentFilters.CanvasType.SVG);
                        pageCanvas.RenderPage(page.item);
                    }

                    var svg = pageCanvasStream.ToArray();
                    canvas.DrawImage(0, 0, svg, "image/svg+xml");
                    canvas.SetPen(System.Drawing.Color.Transparent, 0, 0);
                    canvas.SetBrush(commentsBg);
                    canvas.Rect(System.Drawing.Rectangle.FromLTRB(page.item.Width, 0, page.item.Width + MarginSize, page.item.Height));

                    canvas.SetBrush(System.Drawing.Color.Transparent);
                    canvas.SetPen(commentsBorder, 1, 1);
                    canvas.MoveTo(page.item.Width, 0);
                    canvas.LineTo(page.item.Width, page.item.Height);

                    marginComments = endOfPageComments;
                    endOfPageComments = null;
                }
                else if (CommentsLocation == CommentsLocation.AfterDocument)
                {
                    endOfDocComments.AddRange(endOfPageComments);
                    endOfPageComments = null;
                    canvas.RenderPage(page.item);
                }
                else
                    canvas.RenderPage(page.item);

                // Draw the comments
                if (marginComments?.Count > 0)
                {
                    var overflowComments = Comment.MarginLayout(marginComments, canvas, commentPanel.Width, commentPanel.Height);
                    foreach (var comment in marginComments)
                        comment.Render(canvas, page.item.Width, comment.Top ?? 0, null);

                    if (CommentOverflow == CommentOverflow.NewPage)
                        endOfPageComments = overflowComments.ToList();
                }

                if (endOfPageComments?.Count > 0)
                    Comment.RenderCommentsPage(endOfPageComments, canvas, page.item.Width, page.item.Height);
            }
        }

        if (endOfDocComments?.Count > 0)
            Comment.RenderCommentsPage(endOfDocComments, canvas, (int)8.5 * 96, (int)11 * 96);

        return 0;
    }
    private CanvasType ExtToCanvasType(string? ext)
    {
        CanvasType res;
        if (!Enum.TryParse<CanvasType>(ext?.Trim('.') ?? "", true, out res))
            throw new ArgumentException($"Unknown format {ext}");
        return res;
    }
    static int Main(string[] args) => CommandLineApplication.Execute<Program>(args);
}
