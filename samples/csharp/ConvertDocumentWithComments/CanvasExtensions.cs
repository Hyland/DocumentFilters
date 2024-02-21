using Hyland.DocumentFilters;
using System.Text;
using System.Text.RegularExpressions;

/// <summary>
/// Controls the vertical or horizontal alignment of text or images
/// </summary>
public enum Alignment
{
    /// <summary>
    /// Specifies the text be aligned to the left of the layout, or top when passed as horizontal.
    /// </summary>
    Left,

    /// <summary>
    /// Specifies that text is aligned in the center of the layout rectangle.
    /// </summary>
    Center,

    /// <summary>
    /// Specifies the text be aligned to the right of the layout, or bottom when passed as horizontal.
    /// </summary>
    Right,

    /// <summary>
    /// Alias for Left
    /// </summary>
    Top = Left,

    /// <summary>
    /// Alias for Right
    /// </summary>
    Bottom = Right
}

/// <summary>
/// Hyland.DocumentFilters.Canvas extensions
/// </summary>
public static class CanvasExtensions
{
    public class Measurement
    {
        public System.Drawing.Rectangle Rectangle { get; set; }
        public IList<System.Drawing.Rectangle> Lines { get; set; } = new List<System.Drawing.Rectangle>();
        public System.Drawing.Size Size => Rectangle.Size;
    }

    /// <summary>
    /// Handles splitting text into lines where word-wrapping must occur
    /// </summary>
    /// <param name="canvas"></param>
    /// <param name="text"></param>
    /// <param name="rect"></param>
    /// <returns></returns>
    public static IList<string> WordWrapText(this Canvas canvas, string text, System.Drawing.Rectangle rect)
    {
        IEnumerable<string> SplitIntoWords(string s) =>
            Regex.Split(s, "(?<=[ \\n])");

        var y = rect.Top;
        var x = rect.Left;
        var lineHeight = canvas.TextHeight("Ag");
        var lines = text.Replace("\r", "").Replace("\v", "\n\n").Split(new char[] { '\n', '\x2028' });
        var lineText = new StringBuilder();
        var outputLines = new List<string>();
        foreach (var line in lines)
        {
            var words = SplitIntoWords(line);
            foreach (var word in words)
            {
                var wordWidth = canvas.TextWidth(word);
                if (x + wordWidth > rect.Right)
                {
                    // Add new line
                    outputLines.Add(lineText.ToString());
                    lineText.Clear();
                    y += lineHeight;
                    x = rect.Left;
                }

                x += wordWidth;
                lineText.Append(word);
            }
            outputLines.Add(lineText.ToString());
            lineText.Clear();
            y += lineHeight;
        }
        return outputLines;
    }

    /// <summary>
    /// Aligns the inner rect inside the outer rect using the horz and vert alignment settings
    /// </summary>
    public static System.Drawing.Rectangle AlignRect(this Canvas canvas, System.Drawing.Rectangle outer, System.Drawing.Rectangle inner, Alignment horz, Alignment vert)
    {
        System.Drawing.Rectangle rc = inner;

        rc.Offset(-rc.Left, -rc.Top); // The logic below expects inner to be 0x0
        switch (horz)
        {
            case Alignment.Left:
                rc.Offset(outer.Left, 0);
                break;
            case Alignment.Center:
                rc.Offset(outer.Left + (int)((outer.Width - rc.Width) / 2.0), 0);
                break;
            case Alignment.Right:
                rc.Offset(outer.Right - rc.Width, 0);
                break;
        }

        switch (vert)
        {
            case Alignment.Left:
                rc.Offset(0, outer.Top);
                break;
            case Alignment.Center:
                rc.Offset(0, outer.Top + (int)((outer.Height - rc.Height) / 2.0));
                break;
            case Alignment.Right:
                rc.Offset(0, outer.Bottom - rc.Height);
                break;
        }
        return rc;
    }

    /// <summary>
    /// Measures the specified text string at the specified location while applying word-wrapping.
    /// </summary>
    /// <param name="canvas">Destination canvas to draw to.</param>
    /// <param name="text">Text to be measured.</param>
    /// <param name="rect">Bounding rectangle where text is to be drawn.</param>
    /// <param name="horz">Horizontal alignment within rect</param>
    /// <param name="vert">Vertical alignment within rect</param>
    /// <returns>The bounding rectangle of the text</returns>
    public static Measurement MeasureWordWrapText(this Canvas canvas, string text, System.Drawing.Rectangle rect, Alignment horz = Alignment.Left, Alignment vert = Alignment.Top)
        => HandleWordWrapText(canvas, text, rect, horz, vert, true);

    /// <summary>
    /// Measures the specified text string at the specified location while applying word-wrapping.
    /// </summary>
    /// <param name="canvas">Destination canvas to draw to.</param>
    /// <param name="text">Lines to be measured.</param>
    /// <param name="rect">Bounding rectangle where text is to be drawn.</param>
    /// <param name="horz">Horizontal alignment within rect</param>
    /// <param name="vert">Vertical alignment within rect</param>
    /// <returns>The bounding rectangle of the text</returns>
    public static Measurement MeasureWordWrapText(this Canvas canvas, IEnumerable<string> lines, System.Drawing.Rectangle rect, Alignment horz = Alignment.Left, Alignment vert = Alignment.Top)
        => HandleWordWrapText(canvas, lines.ToList(), rect, horz, vert, true);

    /// <summary>
    /// Draws the specified text string at the specified location while applying word-wrapping.
    /// </summary>
    /// <param name="canvas">Destination canvas to draw to.</param>
    /// <param name="text">Text to be drawn.</param>
    /// <param name="rect">Bounding rectangle where text is to be drawn.</param>
    /// <param name="horz">Horizontal alignment within rect</param>
    /// <param name="vert">Vertical alignment within rect</param>
    /// <param name="measureOnly">If true, the text is not drawn, only the bounding rect is returned.</param>
    /// <returns>The bounding rectangle of the text</returns>
    public static System.Drawing.Rectangle DrawWordWrapText(this Canvas canvas, string text, System.Drawing.Rectangle rect, Alignment horz = Alignment.Left, Alignment vert = Alignment.Top)
        => HandleWordWrapText(canvas, text, rect, horz, vert, false).Rectangle;

    private static Measurement HandleWordWrapText(Canvas canvas, string text, System.Drawing.Rectangle rect, Alignment horz, Alignment vert, bool measureOnly)
    {
        var lines = WordWrapText(canvas, text, rect);
        return HandleWordWrapText(canvas, lines, rect, horz, vert, measureOnly);
    }

    private static Measurement HandleWordWrapText(Canvas canvas, IList<string> lines, System.Drawing.Rectangle rect, Alignment horz, Alignment vert, bool measureOnly)
    {
        var lineHeight = canvas.TextHeight("Ag");

        var x = rect.Left;
        var y = rect.Top;

        // Calculate the vertical position
        if (!measureOnly)
        {
            switch (vert)
            {
                case Alignment.Bottom:
                    y = rect.Bottom - (lineHeight * lines.Count);
                    break;
                case Alignment.Center:
                    y = rect.Top + (int)((rect.Height - (lineHeight * lines.Count)) / 2.0);
                    break;
            }
        }

        // Draw the text
        var lineRects = new List<System.Drawing.Rectangle>();
        foreach (var line in lines)
        {
            var outputRect = new System.Drawing.Rectangle(x, y, rect.Width, lineHeight);
            var textRect = new System.Drawing.Rectangle(x, y, canvas.TextWidth(line), lineHeight);
            var rc = canvas.AlignRect(outputRect, textRect, horz, Alignment.Top);
            if (!measureOnly)
            {
                canvas.TextOut(rc.X, rc.Y, line);
            }
            lineRects.Add(rc);

            y += lineHeight;
        }

        // Return the text position
        return new Measurement
        {
            Rectangle = new System.Drawing.Rectangle(rect.Left, rect.Top, rect.Width, y - rect.Top),
            Lines = lineRects
        };
    }

    /// <summary>
    /// Convert a System.Drawing.Color into a color used by DocFilters 
    /// </summary>
    public static int MakeColor(this Canvas canvas, System.Drawing.Color color)
    {
        return color.R << 16
            | color.G << 8
            | color.B;
    }

    /// <summary>
    /// Sets a solid brush of the specified color onto the canvas
    /// </summary>
    /// <param name="canvas">Canvas where brush is to be applied</param>
    /// <param name="color">The color of brush to create</param>
    public static void SetBrush(this Canvas canvas, System.Drawing.Color color)
    {
        if (canvas == null)
        {
            throw new ArgumentNullException("canvas");
        }
        else if (color.IsEmpty)
        {
            canvas.SetBrush(0, 0);
        }
        else
        {
            canvas.SetBrush(canvas.MakeColor(color), 1);
        }
    }

    /// <summary>
    /// Sets a solid pen of the specified color onto the canvas
    /// </summary>
    /// <param name="canvas">Canvas where pen is to be applied</param>
    /// <param name="color">The color of pen to create</param>
    /// <param name="width">The width of the pen in pixels</param>
    /// <param name="style">Reserved</param>
    public static void SetPen(this Canvas canvas, System.Drawing.Color color, int width, int style)
    {
        if (canvas == null)
        {
            throw new ArgumentNullException("canvas");
        }
        else if (color.IsEmpty)
        {
            canvas.SetPen(0, 0, 0);
        }
        else
        {
            canvas.SetPen(canvas.MakeColor(color), width, style);
        }
    }

}
