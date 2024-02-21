using Hyland.DocumentFilters;
using Hyland.DocumentFilters.Annotations;
using System.Xml.Schema;

public enum CommentsLocation
{
    None, Margin, AfterPage, AfterDocument
}
public enum CommentOverflow
{
    Discard, NewPage
}

internal class Comment
{
    public class Options
    {
        public string Font { get; set; } = "Arial";
        public int FontSize { get; set; } = 9;
        public System.Drawing.Color Color { get; set; } = System.Drawing.Color.Black;


        public string? AuthorFont { get; set; }
        public int? AuthorFontSize { get; set; }
        public int AuthorFontStyle { get; set; } = ISYS11dfConstants.IGR_TEXT_STYLE_BOLD;
        public System.Drawing.Color? AuthorColor { get; set; }

        public string? FooterFont { get; set; }
        public int? FooterFontSize { get; set; }
        public int FooterFontStyle { get; set; } = ISYS11dfConstants.IGR_TEXT_STYLE_ITALIC;
        public System.Drawing.Color? FooterColor { get; set; }
        public bool DrawLine { get; set; } = true;

        public System.Drawing.Rectangle Margins { get; set; } = System.Drawing.Rectangle.FromLTRB(48, 48, 48, 48);
    }

    public string? Author { get; private set; }
    public string? Body { get; private set; }
    public string? Footer { get; private set; }
    public System.Drawing.Size BodySize { get; private set; }
    public System.Drawing.Size CaptionSize { get; private set; }
    public System.Drawing.Size FooterSize { get; private set; }
    public System.Drawing.Size BoxSize { get; private set; }
    public int TextMargin { get; private set; } = 5;
    public int LeftMargin { get; private set; } = 30;
    public int RightMargin { get; private set; } = 5;
    public Annotation Annotation { get; private set; }
    public int? Top { get; set; }
    public int Bottom => (Top ?? 0) + BoxSize.Height;
    public int Height => BoxSize.Height;
    public bool? DrawLine { get; set; }
    public string? Location { get; set; }
    public bool? Continued { get; set; }

    public Comment(Annotation annotation, bool reply = false)
    {
        Body = annotation.Text.Trim(new char[] { '\n', '\r', '\v' });
        Author = annotation.Title;
        if (annotation.DateCreated.Year > 1970)
            Footer = $"{annotation.DateCreated:d}";
        Annotation = annotation;

        if (reply)
        {
            LeftMargin += 15;
            DrawLine = false;
        }
    }

    public Comment CalculateSize(Canvas canvas, int maxWidth, Comment.Options? opts = null)
    {
        opts ??= new Options();

        var rect = new System.Drawing.Rectangle(0, 0, maxWidth - (LeftMargin + RightMargin + (TextMargin * 2)), 4000);

        if (!string.IsNullOrWhiteSpace(Author))
        {
            canvas.SetFont(opts.AuthorFont ?? opts.Font, opts.AuthorFontSize ?? opts.FontSize, opts.AuthorFontStyle);
            CaptionSize = canvas.MeasureWordWrapText(Author, rect, Alignment.Left, Alignment.Top).Size;
        }
        if (!string.IsNullOrWhiteSpace(Footer))
        {
            canvas.SetFont(opts.FooterFont ?? opts.Font, opts.FooterFontSize ?? opts.FontSize, opts.FooterFontStyle);
            FooterSize = canvas.MeasureWordWrapText(Footer, rect, Alignment.Left, Alignment.Top).Size;
        }

        canvas.SetFont(opts.Font, opts.FontSize, 0);
        BodySize = canvas.MeasureWordWrapText(Body ?? "", rect, Alignment.Left, Alignment.Top).Size;


        BoxSize = new System.Drawing.Size(maxWidth - (LeftMargin + RightMargin), CaptionSize.Height + BodySize.Height + FooterSize.Height + (TextMargin * 2));
        return this;
    }

    public void Render(Canvas canvas, int x, int y, Comment.Options? opts)
    {
        opts ??= new Options();

        var textRect = new System.Drawing.Rectangle(x + LeftMargin + TextMargin, y + TextMargin, BodySize.Width, BodySize.Height);

        canvas.SetBrush(System.Drawing.Color.Transparent);
        canvas.SetPen(System.Drawing.Color.Blue, 1, 1);
        canvas.RoundRect(x + LeftMargin, y, x + LeftMargin + BoxSize.Width, y + BoxSize.Height, 6);

        canvas.SetPen(0, 0, 0);
        canvas.SetBrush(System.Drawing.Color.Black);
        if (!string.IsNullOrWhiteSpace(Author))
        {
            string t = Author;
            if (Continued.GetValueOrDefault(false))
                t += " (cont)";
            canvas.SetFont(opts.AuthorFont ?? opts.Font, opts.AuthorFontSize ?? opts.FontSize, opts.AuthorFontStyle);
            canvas.SetBrush(opts.AuthorColor ?? opts.Color);
            canvas.DrawWordWrapText(t, textRect, Alignment.Left, Alignment.Top);

            textRect = new System.Drawing.Rectangle(x + LeftMargin + TextMargin, y + TextMargin + CaptionSize.Height, BodySize.Width, BodySize.Height);
        }

        if (!string.IsNullOrWhiteSpace(Body))
        {
            canvas.SetFont(opts.Font, opts.FontSize, 0);
            canvas.SetBrush(opts.Color);
            canvas.DrawWordWrapText(Body, textRect, Alignment.Left, Alignment.Top);
        }

        if (!string.IsNullOrWhiteSpace(Footer))
        {
            textRect = new System.Drawing.Rectangle(x + LeftMargin + TextMargin, y + TextMargin + CaptionSize.Height + BodySize.Height, FooterSize.Width, FooterSize.Height);

            canvas.SetFont(opts.FooterFont ?? opts.Font, opts.FooterFontSize ?? opts.FontSize, opts.FooterFontStyle);
            canvas.SetBrush(opts.FooterColor ?? opts.Color);
            canvas.DrawWordWrapText(Footer, textRect, Alignment.Left, Alignment.Top);
        }

        if (DrawLine ?? opts.DrawLine)
        {
            canvas.SetPen(System.Drawing.Color.Blue, 1, 1);
            canvas.MoveTo(x + LeftMargin, y + TextMargin * 2);
            canvas.LineTo(x - TextMargin, Annotation.Rect.Bottom);
            canvas.LineTo(Annotation.Rect.Left, Annotation.Rect.Bottom);
        }
    }

    public Comment? Split(Canvas canvas, int maxWidth, int maxHeight, Options? opts)
    {
        opts ??= new Options();

        var rect = new System.Drawing.Rectangle(0, CaptionSize.Height, maxWidth - (LeftMargin + RightMargin + (TextMargin * 2)), 4000);

        //var y = ;
        var bodyLines = canvas.WordWrapText(Body ?? "", rect);
        var bodyLinesHeights = canvas.MeasureWordWrapText(bodyLines, rect).Lines;

        if (bodyLines.Count == bodyLinesHeights.Count)
        {
            for (int i = 0, c = bodyLines.Count; i < c; ++i)
            {
                if (bodyLinesHeights[i].Bottom > maxHeight - FooterSize.Height)
                {
                    var keepLines = bodyLines.Take(i - 1);
                    var removeLines = bodyLines.Skip(i - 1);
                    var res = new Comment(Annotation)
                    {
                        Author = Author,
                        Footer = Footer,
                        LeftMargin = LeftMargin,
                        Location = Location,
                        Continued = true,
                        Body = string.Join(" ", removeLines)
                    };
                    Body = string.Join(" ", keepLines);
                    Footer = "...";
                    BoxSize = new System.Drawing.Size(BoxSize.Width, bodyLinesHeights[i - 1].Bottom + 10);
                    BodySize = new System.Drawing.Size(BodySize.Width, bodyLinesHeights[i - 1].Top - bodyLinesHeights[0].Top);

                    return res;
                }
            }
        }

        return null;
    }

    public static IEnumerable<Comment> MarginLayout(IList<Comment> comments, Canvas canvas, int panelWidth, int pageHeight, Options? opts = null)
    {
        if (comments.Count == 0)
            return comments;

        opts ??= new Options();

        // A comment should be placed as close to the line containing it as possible.
        for (int i = 0, c = comments.Count; i < c; ++i)
        {
            var me = comments[i];
            var prev = i > 0 ? comments[i - 1] : null;

            me.CalculateSize(canvas, panelWidth);

            int y = me.Annotation.Rect.Bottom - 15;
            if (i > 0 && prev != null && y <= prev.Bottom)
                y = prev.Bottom + 5;
            if (y + me.BoxSize.Height > pageHeight)
            {
                me.Top = pageHeight - me.Height - 15;

                // Comment is off the bottom of the page.
                // Walk backwards to push previous comments up.

                for (int j = i - 1; j >= 0; --j)
                {
                    var self = comments[j];
                    var next = comments[j + 1];

                    if (self.Bottom > next.Top)
                        self.Top = next.Top - self.Height - 5;
                }

            }
            else
                me.Top = y;
        }

        if (comments[0].Top < 0)
        {
            // If we moved content off the page, we need to shift back down, and split/move the comment at the bottom.
            var shift = Math.Abs(comments[0].Top ?? 0) + 10;
            for (int i = 0, c = comments.Count; i < c; ++i)
            {
                var self = comments[i];
                self.Top += shift;
                if (self.Bottom > pageHeight)
                {
                    var availableHeight = pageHeight - (self.Top ?? 0) - 20;
                    if (availableHeight < self.CaptionSize.Height * 2)
                    {
                        // move the entire comment
                    }
                    else
                    {
                        // Create a list of comments that won't fit
                        List<Comment> splitComments = new();
                        var split = self.Split(canvas, panelWidth, availableHeight, opts);
                        if (split != null)
                            splitComments.Add(split);
                        splitComments.AddRange(comments.Skip(i + 1));
                        // And remove them from the inbound list
                        while (comments.Count > i + 1)
                            comments.RemoveAt(comments.Count - 1);

                        return splitComments;
                    }
                }
            }
        }

        return new Comment[] { };
    }

    public static void RenderCommentsPage(IEnumerable<Comment> comments, Canvas canvas, int pageWidth, int pageHeight, Options? opts = null)
    {
        opts ??= new Options();
        opts.DrawLine = false;

        int y = opts.Margins.Top;
        int x = opts.Margins.Left;
        int printableWidth = pageWidth - (opts.Margins.Left + opts.Margins.Right);
        int printableHeight = pageHeight - (opts.Margins.Top + opts.Margins.Bottom);
        string? lastLocation = null;
        foreach (var comment in comments.Select((item, index) => new { item, index }))
        {
            if (comment.index == 0)
                canvas.BlankPage(pageWidth, pageHeight);

            comment.item.CalculateSize(canvas, printableWidth);
            if (y + comment.item.Height > printableHeight)
            {
                y = opts.Margins.Top;
                lastLocation = null;
                canvas.BlankPage(pageWidth, pageHeight);
            }

            if (comment.item.Location != null && comment.item.Location != lastLocation)
            {
                canvas.SetFont(opts.Font, (int)(opts.FontSize * 1.2), 1);
                canvas.SetBrush(System.Drawing.Color.DarkGray);
                canvas.TextOut(x, y, comment.item.Location);

                y += canvas.TextHeight(comment.item.Location);
                canvas.SetPen(System.Drawing.Color.DarkGray, 1, 1);
                canvas.SetBrush(System.Drawing.Color.Transparent);
                canvas.MoveTo(x, y);
                canvas.LineTo(pageWidth - opts.Margins.Right, y);
                y += 5;

                lastLocation = comment.item.Location;
            }

            comment.item.Render(canvas, x, y, opts);

            y += comment.item.Height + 5;
        }
    }
}