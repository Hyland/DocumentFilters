//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Security.Cryptography;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// The Canvas interface allows rendering of pages to a variety of output devices, including HD HTML, PNG, and PDF.
    /// 
    /// The Canvas object also allows post-processing / image manipulation of output such as annotations, redaction, bates stamping, or general drawing.
    /// 
    /// To obtain this interface, call the DocumentFilters.MakeOutputCanvas or DocumentFilters.MakeOutputCanvasOnStream methods.
    /// </summary>
    public class Canvas : DocumentFiltersBase, IDisposable
    {
        private int _handle = -1;
        private CanvasType _type = 0;
        private string _options = null;
        private string _filename = null;
        private IGRStream _stream = null;
        private IntPtr _streamHandle = IntPtr.Zero;
        private readonly object _disposeSyncRoot = new object();

        /// <summary>
        /// Internal constructor for creating a canvas object.
        /// </summary>
        internal Canvas(DocumentFiltersBase parent, string filename, CanvasType type, string options)
        {
            _type = type;
            _options = options;
            _filename = filename;
        }

        /// <summary>
        /// Internal constructor for creating a canvas object.
        /// </summary>
        internal Canvas(DocumentFiltersBase parent, System.IO.Stream stream, CanvasType type, string options)
        {
            _type = type;
            _options = options;
            _stream = new IGRStreamBridge(stream);
            _streamHandle = _stream.NewWritableStreamPtr();
        }

        /// <summary>
        /// Internal constructor for creating a canvas object.
        /// </summary>
        internal Canvas(DocumentFiltersBase parent, IGRStream stream, CanvasType type, string options)
        {
            _type = type;
            _options = options;
            _stream = stream;
            _streamHandle = stream.NewWritableStreamPtr();
        }

        /// <summary>
        /// Destructor for the Canvas object.
        /// </summary>
        ~Canvas()
        {
            Dispose();
        }

        /// <summary>
        /// Disposes of the Canvas object.
        /// </summary>
        public virtual void Dispose()
        {
            Close();
        }

        /// <summary>
        /// Disposes of the Canvas object.
        /// </summary>
        void IDisposable.Dispose()
        {
            Close();
        }

        /// <summary>
        /// Returns the IGR_HCANVAS handle for the canvas.
        /// </summary>
        public int Handle
        {
            get
            {
                if (_handle == -1)
                {
                    _handle = 0;
                    if (!String.IsNullOrEmpty(_filename))
                    {
                        Check(ISYS11df.IGR_Make_Output_Canvas((int) _type, _filename, _options, ref _handle, ref ecb), ecb);
                    }
                    else
                    {
                        Check(ISYS11df.IGR_Make_Output_Canvas_On((int) _type, _streamHandle, _options, ref _handle, ref ecb));
                    }                    
                }

                if (_handle > 0)
                {
                    return _handle;
                }
                throw new IGRException(4, "Canvas has been closed");
            }
        }

        /// <summary>
        /// Close the canvas object, disposing of any resources.
        /// </summary>
        public void Close()
        {
            lock (_disposeSyncRoot)
            {
                if (_handle > 0)
                {
                    Check(ISYS11df.IGR_Close_Canvas(_handle, ref ecb));
                    _handle = 0;
                }
                if (_streamHandle != IntPtr.Zero)
                {
                    IGRStream.DestroyStreamPtr(_streamHandle);
                    _streamHandle = IntPtr.Zero;
                }
            }
        }

        /// <summary>
        /// Render a page to the canvas using default options.
        /// </summary>
        /// <param name="page">The page to be rendered</param>
        public void RenderPage(Page page)
        {
            VerifyArgumentNotNull(page, "page");
            Check(ISYS11df.IGR_Render_Page(page.Handle, Handle, ref ecb));
        }

        /// <summary>
        /// Render a page to the canvas using the specified options.
        /// </summary>
        /// <param name="page">The page to be rendered</param>
        /// <param name="options">Canvas processing options to use when creating the new page.</param>
        public void RenderPage(Page page, string options)
        {
            VerifyArgumentNotNull(page, "page");
            var page_render_props = new IGR_Render_Page_Properties();
            Check(ISYS11df.IGR_Render_Page_Ex(page.Handle, Handle, options, ref page_render_props, ref ecb));
        }

        /// <summary>
        /// Create a blank page on the canvas with the specified properties.
        /// </summary>
        /// <param name="width">The width of the new page, in  pixels.</param>
        /// <param name="height">The height of the new page, in pixels.</param>
        /// <param name="options">Canvas processing options to use when creating the new page.</param>
        public void BlankPage(int width, int height, string options = "")
        {
            VerifyArgumentInRange(width, 1, 0xffff, "width");
            VerifyArgumentInRange(height, 1, 0xffff, "height");

            Check(ISYS11df.IGR_Canvas_Blank_Page(Handle, options, width, height, IntPtr.Zero, ref ecb), ecb);
        }

        /// <summary>
        /// Internal class for managing HGlobal memory.
        /// </summary>
        private class DisposableHGlobal
            : IDisposable
        {
            public IntPtr ptr;

            /// <summary>
            /// Constructor for the DisposableHGlobal class.
            /// </summary>
            public DisposableHGlobal(IntPtr ptr)
            {
                this.ptr = ptr;
            }

            /// <summary>
            /// Dispose of the DisposableHGlobal object.
            /// </summary>
            public void Dispose()
            {
                Marshal.FreeHGlobal(ptr);
            }
        }

        /// <summary>
        /// Render the page to the canvas with the specified properties.
        /// </summary>
        /// <param name="page">The page to render.</param>
        /// <param name="options">Canvas options to use when rendering page.</param>
        /// <param name="renderPageProperties">Render properties to use when rendering the page.</param>
        public void RenderPage(Page page, string options, RenderPageProperties renderPageProperties)
        {
            VerifyArgumentNotNull(page, "page");

            var page_render_props = new IGR_Render_Page_Properties
            {
                size = (uint)Marshal.SizeOf(typeof(IGR_Render_Page_Properties)),
                form_values = new IntPtr(0),
                form_value_count = renderPageProperties == null ? 0 : (uint)renderPageProperties.FormValues.Count,
                redaction_flags = renderPageProperties == null ? 0 : (uint)renderPageProperties.RedactionFlags,
                redaction_count = renderPageProperties == null ? 0 : (uint)renderPageProperties.Redactions.Count,
                redactions = new IntPtr(0),
                source_rect = new IGR_Rect(renderPageProperties?.SourceRect ?? Rectangle.Empty),
                dest_rect = new IGR_Rect(renderPageProperties?.DestRect ?? Rectangle.Empty)
            };

            int formElementSize = Marshal.SizeOf(typeof(IGR_Render_Page_Form_Values));
            int redactElementSize = Marshal.SizeOf(typeof(IGR_Render_Page_Redactions));
            using (var formElements = new DisposableHGlobal(Marshal.AllocHGlobal(formElementSize * (int)page_render_props.form_value_count)))
            using (var redactionElements = new DisposableHGlobal(Marshal.AllocHGlobal(redactElementSize * (int)page_render_props.redaction_count)))
            {
                for (int i = 0; i < (int)page_render_props.form_value_count; ++i)
                {
                    Marshal.StructureToPtr(renderPageProperties.FormValues[i], new IntPtr(formElements.ptr.ToInt64() + (i * formElementSize)), false);
                }

                for (var i = 0; i < page_render_props.redaction_count; ++i)
                    Marshal.StructureToPtr(renderPageProperties.Redactions[i], new IntPtr(redactionElements.ptr.ToInt64() + (i * redactElementSize)), false);

                page_render_props.form_values = formElements.ptr;
                page_render_props.redactions = redactionElements.ptr;
                Check(ISYS11df.IGR_Render_Page_Ex(page.Handle, Handle, options, ref page_render_props, ref ecb));
            }
        }

        /// <summary>
        /// Render all pages of the extractor to the canvas.
        /// </summary>
        /// <param name="extractor">The extractor containing the pages to render.</param>
        public void RenderPages(Extractor extractor)
        {
            VerifyArgumentNotNull(extractor, "extractor");

            foreach (var page in extractor.Pages) using (page)
            {
                RenderPage(page);
            }
        }

        /// <summary>
        /// Add a custom metadata field to the output canvas. This function MUST be called before the first call to RenderPage.
        /// </summary>
        /// <param name="name">The name of the metadata field.</param>
        /// <param name="value">The value of the metadata field.</param>
        public void AddCustomMetadata(string name, string value)
        {
            VerifyArgumentNotEmpty(name, "name");
            VerifyArgumentNotEmpty(value, "value");
            if (_handle != -1)
                throw new IGRException(13, "Metadata must be added before any drawing is attempt");
            _options += $"META(\"{name}\", \"{value}\");";
        }

        /// <summary>
        /// The Arc method draws an arc on the image along the perimeter of the ellipse, bounded by the specified rectangle. It uses the current Pen.
        /// </summary>
        /// <param name="x">Left-most coordinate of the bounding box.</param>
        /// <param name="y">Top-most coordinate of the bounding box.</param>
        /// <param name="x2">Right-most coordinate of the bounding box.</param>
        /// <param name="y2">Bottom-most coordinate of the bounding box.</param>
        /// <param name="x3">X coordinate of the start point.</param>
        /// <param name="y3">Y coordinate of the start point.</param>
        /// <param name="x4">X coordinate of the end point.</param>
        /// <param name="y4">Y coordinate of the end point.</param>
        public void Arc(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4)
        {
            Check(ISYS11df.IGR_Canvas_Arc(Handle, x, y, x2, y2, x3, y3, x4, y4, ref ecb));
        }

        /// <summary>
        /// The AngleArc method draws an arc on the image centered at the specified point, with the specified radius, starting at the specified angle and extending for the specified sweep angle. It uses the current Pen.
        /// </summary>
        /// <param name="x1">The X center point.</param>
        /// <param name="y1">The Y center point.</param>
        /// <param name="radius">The radius of the circle.</param>
        /// <param name="startAngle">The start angle.</param>
        /// <param name="sweepAngle">The sweep angle.</param>
        public void AngleArc(int x1, int y1, int radius, int startAngle, int sweepAngle)
        {
            Check(ISYS11df.IGR_Canvas_AngleArc(Handle, x1, y1, radius, startAngle, sweepAngle, ref ecb));
        }

        /// <summary>
        /// The Chord method draws a closed figure represented by the intersection of a line and an ellipse. The ellipse is bisected by a line that runs between X3, Y3 and X4, Y4 coordinates.
        /// </summary>
        /// <param name="x">Left-most coordinate of the bounding box.</param>
        /// <param name="y">Top-most coordinate of the bounding box.</param>
        /// <param name="x2">Right-most coordinate of the bounding box.</param>
        /// <param name="y2">Bottom-most coordinate of the bounding box.</param>
        /// <param name="x3">X coordinate of the start point.</param>
        /// <param name="y3">Y coordinate of the start point.</param>
        /// <param name="x4">X coordinate of the end point.</param>
        /// <param name="y4">Y coordinate of the end point.</param>
        public void Chord(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4)
        {
            Check(ISYS11df.IGR_Canvas_Chord(Handle, x, y, x2, y2, x3, y3, x4, y4, ref ecb));
        }

        /// <summary>
        /// The Ellipse method draws the ellipse defined by a bounding rectangle on the canvas, outlined with the current pen and filled with the current brush.
        /// </summary>
        /// <param name="x">Left-most coordinate of the bounding box.</param>
        /// <param name="y">Top-most coordinate of the bounding box.</param>
        /// <param name="x2">Right-most coordinate of the bounding box.</param>
        /// <param name="y2">Bottom-most coordinate of the bounding box.</param>
        public void Ellipse(int x, int y, int x2, int y2)
        {
            Check(ISYS11df.IGR_Canvas_Ellipse(Handle, x, y, x2, y2, ref ecb));
        }

        /// <summary>
        /// The Rect method draws a rectangle using the Brush and Pen of the canvas to fill and draw the border.
        /// </summary>
        /// <param name="left">Left-most coordinate of the bounding box.</param>
        /// <param name="top">Top-most coordinate of the bounding box.</param>
        /// <param name="right">Right-most coordinate of the bounding box.</param>
        /// <param name="bottom">Bottom-most coordinate of the bounding box.</param>
        public void Rect(int left, int top, int right, int bottom)
        {
            Check(ISYS11df.IGR_Canvas_Rect(Handle, left, top, right, bottom, ref ecb));
        }

        /// <summary>
        /// The Rect method draws a rectangle using the Brush and Pen of the canvas to fill and draw the border.
        /// </summary>
        /// <param name="rect">The rectangle to draw.</param>
        public void Rect(System.Drawing.Rectangle rect)
        {
            Rect(rect.Left, rect.Top, rect.Right, rect.Bottom);
        }

        /// <summary>
        /// LineTo draws a line on the canvas from current pen position to the point specified by X and Y, and sets the pen position to (X, Y) coordinates.
        /// </summary>
        /// <param name="x">The X coordinate for the new pen position.</param>
        /// <param name="y">The Y coordinate for the new pen position.</param>
        public void LineTo(int x, int y)
        {
            Check(ISYS11df.IGR_Canvas_LineTo(Handle, x, y, ref ecb));
        }

        /// <summary>
        /// MoveTo changes the current drawing position to the point (X, Y).
        /// </summary>
        /// <param name="x">The X coordinate for the new pen position.</param>
        /// <param name="y">The Y coordinate for the new pen position.</param>
        public void MoveTo(int x, int y)
        {
            Check(ISYS11df.IGR_Canvas_MoveTo(Handle, x, y, ref ecb));
        }

        /// <summary>
        /// The Pie method draws a pie-shaped section of the ellipse on the canvas, bounded by the rectangle (X, Y) and (X2, Y2).
        /// </summary>
        /// <param name="x">Left-most coordinate of the bounding box.</param>
        /// <param name="y">Top-most coordinate of the bounding box.</param>
        /// <param name="x2">Right-most coordinate of the bounding box.</param>
        /// <param name="y2">Bottom-most coordinate of the bounding box.</param>
        /// <param name="x3">X coordinate of the start point.</param>
        /// <param name="y3">Y coordinate of the start point.</param>
        /// <param name="x4">X coordinate of the end point.</param>
        /// <param name="y4">Y coordinate of the end point.</param>
        public void Pie(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4)
        {
            Check(ISYS11df.IGR_Canvas_Pie(Handle, x, y, x2, y2, x3, y3, x4, y4, ref ecb));
        }

        /// <summary>
        /// RoundRect draws a rectangle with rounded corners on the canvas, outlined with the current pen and filled with the current brush.
        /// </summary>
        /// <param name="x">Left-most coordinate of the bounding box.</param>
        /// <param name="y">Top-most coordinate of the bounding box.</param>
        /// <param name="x2">Right-most coordinate of the bounding box.</param>
        /// <param name="y2">Bottom-most coordinate of the bounding box.</param>
        /// <param name="radius">The radius to use for the rounded corner.</param>
        public void RoundRect(int x, int y, int x2, int y2, int radius)
        {
            Check(ISYS11df.IGR_Canvas_RoundRect(Handle, x, y, x2, y2, radius, ref ecb));
        }

        /// <summary>
        /// TextOut writes a string on the canvas, starting at (X, Y). It updates the pen position to the end of the string and uses the current font and brush.
        /// </summary>
        /// <param name="x">Left-most coordinate of the bounding box.</param>
        /// <param name="y">Top-most coordinate of the bounding box.</param>
        /// <param name="text">The text to output to the canvas.</param>
        public void TextOut(int x, int y, string text)
        {
            Check(ISYS11df.IGR_Canvas_TextOut(Handle, x, y, text, ref ecb));
        }

        /// <summary>
        /// Writes a string inside a clipping rectangle, using the current brush and font.
        /// </summary>
        /// <param name="x">Left-most coordinate of the bounding box.</param>
        /// <param name="y">Top-most coordinate of the bounding box.</param>
        /// <param name="x2">Right-most coordinate of the bounding box.</param>
        /// <param name="y2">Bottom-most coordinate of the bounding box.</param>
        /// <param name="text">The text to output to the canvas.</param>
        /// <param name="flags">Reserved for future use.</param>
        public void TextRect(int x, int y, int x2, int y2, string text, int flags)
        {
            Check(ISYS11df.IGR_Canvas_TextRect(Handle, x, y, x2, y2, text, flags, ref ecb));
        }

        /// <summary>
        /// Returns the width in pixels, of a string if rendered with the current font.
        /// </summary>
        /// <param name="text">A string containing the text to be measured.</param>
        /// <returns>Integer expressing the width or height.</returns>
        public int TextWidth(string text)
        {
            int width = 0, height = 0;
            Check(ISYS11df.IGR_Canvas_MeasureText(Handle, text, ref width, ref height, ref ecb));
            return width;
        }

        /// <summary>
        /// Returns the height in pixels, of a string if rendered with the current font.
        /// </summary>
        /// <param name="text">A string containing the text to be measured.</param>
        /// <returns>Integer expressing the width or height.</returns>
        public int TextHeight(string text)
        {
            int width = 0, height = 0;
            Check(ISYS11df.IGR_Canvas_MeasureText(Handle, text, ref width, ref height, ref ecb));
            return height;
        }

        /// <summary>
        /// Map a System.Drawing.Color to a 32-bit integer.
        /// </summary>
        private static int MapColor(System.Drawing.Color color) => color.A << 24 | color.R << 16 | color.G << 8 | color.B;

        /// <summary>
        /// SetPen updates the canvas pen with the specific color, width, and style.
        /// </summary>
        /// <param name="color">The color expressed as a 32-bit integer.</param>
        /// <param name="width">The width of the pen, expressed in points.</param>
        /// <param name="style">The pen style used when drawing</param>
        public void SetPen(int color, int width, int style)
        {
            Check(ISYS11df.IGR_Canvas_SetPen(Handle, color, width, style, ref ecb));
        }

        /// <summary>
        /// SetPen updates the canvas pen with the specific color, width, and style.
        /// </summary>
        /// <param name="color">The color expressed as a System.Drawing.Color</param>
        /// <param name="width">The width of the pen, expressed in points.</param>
        /// <param name="style">The pen style used when drawing</param>
        public void SetPen(System.Drawing.Color color, int width = 1, int style = 1) => SetPen(MapColor(color), width, style);

        /// <summary>
        /// SetBrush updates the current brush on the canvas with the given color and style. Brushes are used when drawing rectangles and text.
        /// </summary>
        /// <param name="color">The color expressed as a 32-bit integer.</param>
        /// <param name="style">The brush style used when drawing.</param>
        public void SetBrush(int color, int style)
        {
            Check(ISYS11df.IGR_Canvas_SetBrush(Handle, color, style, ref ecb));
        }

        /// <summary>
        /// SetBrush updates the current brush on the canvas with the given color and style. Brushes are used when drawing rectangles and text.
        /// </summary>
        /// <param name="color">The color expressed as a System.Drawing.Color</param>
        /// <param name="style">The brush style used when drawing.</param>
        public void SetBrush(System.Drawing.Color color, int style = 1) => SetBrush(MapColor(color), style);

        /// <summary>
        /// SetFont specifies the font to be used when drawing text to the canvas. All subsequent calls to TextOut and MeasureText will use this font.
        /// </summary>
        /// <param name="name">Font Family name to use; this is the font display name such as Arial or 'Courier New.'</param>
        /// <param name="size">The size to render the font.</param>
        /// <param name="style">A bitmask of style information.</param>
        public void SetFont(string name, int size, int style)
        {
            Check(ISYS11df.IGR_Canvas_SetFont(Handle, name, size, style, ref ecb));
        }

        /// <summary>
        /// SetOpacity sets the opacity or transparency for future drawing routines.
        /// </summary>
        /// <param name="opacity">The opacity expressed from 0 to 255.</param>
        public void SetOpacity(int opacity)
        {
            Check(ISYS11df.IGR_Canvas_SetOpacity(Handle, (byte) opacity, ref ecb));
        }

        /// <summary>
        /// DrawImage renders an image from a buffer onto the Canvas.
        /// </summary>
        /// <param name="x">Left-most coordinate of the image bounding box.</param>
        /// <param name="y">Top-most coordinate of the image bounding box.</param>
        /// <param name="imagedata">Binary data of the image.</param>
        /// <param name="mimetype">Describes the format of the image data.</param>
        public void DrawImage(int x, int y, byte[] imagedata, string mimetype)
        {
            Check(ISYS11df.IGR_Canvas_DrawImage(Handle, x, y, imagedata, new IntPtr(imagedata.Length), mimetype, ref ecb));
        }

        /// <summary>
        /// DrawScaleImage renders an image from a buffer onto the Canvas. The image is scaled to a specified size.
        /// </summary>
        /// <param name="x">Left-most coordinate of the image bounding box.</param>
        /// <param name="y">Top-most coordinate of the image bounding box.</param>
        /// <param name="width">Desired width of the rendered image in pixels.</param>
        /// <param name="height">Desired height of the rendered image in pixels.</param>
        /// <param name="imagedata">Binary data of the image.</param>
        /// <param name="mimetype">Describes the format of the image data.</param>
        public void DrawScaleImage(int x, int y, int width, int height, byte[] imagedata, string mimetype)
        {
            Check(ISYS11df.IGR_Canvas_DrawScaleImage(Handle, x, y, width, height, imagedata, new IntPtr(imagedata.Length), mimetype, ref ecb));
        }

        /// <summary>
        /// Sets the rotation of future drawing routines.
        /// </summary>
        /// <param name="degrees">The rotation in degrees.</param>
        public void Rotation(int degrees)
        {
            Check(ISYS11df.IGR_Canvas_Rotation(Handle, degrees, ref ecb));
        }

        /// <summary>
        /// Reset the canvas to its default state.
        /// </summary>
        public void Reset()
        {
            Check(ISYS11df.IGR_Canvas_Reset(Handle, ref ecb));
        }

        /// <summary>
        /// The Annotate method draws the given annotation onto the current page. The annotation is delivered as a string of JSON text.
        /// </summary>
        /// <param name="annotation">The annotation object to render.</param>
        public void Annotate(Annotations.Annotation annotation)
        {
            Annotate(annotation?.Serialize());
        }

        /// <summary>
        /// The Annotate method draws the given annotation onto the current page. The annotation is delivered as a string of JSON text.
        /// </summary>
        /// <param name="JSON">The annotation to render, expressed as JSON.</param>
        public void Annotate(string JSON)
        {
            if (!string.IsNullOrEmpty(JSON))
            {
                Check(ISYS11df.IGR_Canvas_Annotate_JSON(Handle, JSON, ref ecb));
            }
        }

        /// <summary>
        /// Remove existing bookmarks from the canvas.
        /// </summary>
        public void ClearBookmarks()
        {
            Check(ISYS11df.IGR_Canvas_Bookmarks_Clear(Handle, ref ecb));
        }

        /// <summary>
        /// Appends only this current bookmark. 
        /// This function does not enumerate the <see cref="Bookmark.Children"/>
        /// The <see cref="Bookmark.Level"/> must not be more than 1 greater than the previously appended bookmark
        /// </summary>
        /// <param name="bookmark"></param>
        public void AppendBookmark(Bookmark bookmark)
        {
            Check(ISYS11df.IGR_Canvas_Bookmarks_Append(Handle, ref bookmark._item, ref ecb));
        }

        /// <summary>
        /// Appends this bookmark and the <see cref="Bookmark.Children"/> recursively
        /// </summary>
        /// <exception cref="ArgumentOutOfRangeException">Thrown if a <see cref="Bookmark.Level"/> is incorrect</exception>
        /// <remarks>If an exception is thrown this function does not call <see cref="ClearBookmarks"/> and may leave a partial set of bookmarks</remarks>
        /// <param name="bookmarkRoot"></param>
        public void AppendBookmarkRecursive(Bookmark bookmarkRoot)
        {
            var bookmarks = new Stack<Bookmark>();
            bookmarks.Push(bookmarkRoot);

            int currentLevel = 0;

            while (bookmarks.Count > 0)
            {
                var top = bookmarks.Pop();

                if (top.Level < 0 || top.Level > currentLevel + 1)
                    throw new ArgumentOutOfRangeException(nameof(Bookmark.Level), $"Invalid {nameof(Bookmark.Level)} for {nameof(Bookmark)} with {nameof(Bookmark.Title)} \"{top.Title}\"");

                currentLevel = top.Level;

                AppendBookmark(top);

                // This ensures that the total order is depth first while maintaining the original bookmark order
                // Consider the following data
                // 1
                // 1.1
                // 1.1.1
                // 1.1.2
                // 1.2
                // 1.3

                // The total order would be 
                // push(1)
                // pop(1)
                // push(1.3)
                // push(1.2)
                // push(1.1)
                // pop(1.1)
                // push(1.1.2)
                // push(1.1.1)
                // pop(1.1.1)
                // pop(1.1.2)
                // pop(1.2)
                // pop(1.3)
                foreach (var child in EnumerableExtensions.ReverseIterator(top.Children))
                {
                    bookmarks.Push(child);
                }
            }
        }
    }
}
