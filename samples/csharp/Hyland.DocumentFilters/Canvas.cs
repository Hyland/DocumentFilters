//===========================================================================
// (c) 2018 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;

namespace Hyland.DocumentFilters
{

    public class Canvas : DocumentFiltersBase, IDisposable
    {
        private int _handle = -1;
        private CanvasType _type = 0;
        private string _options = null;
        private string _filename = null;
        private IGRStream _stream = null;
        private IntPtr _streamHandle = IntPtr.Zero;

        internal Canvas(DocumentFiltersBase parent, string filename, CanvasType type, string options)
        {
            _type = type;
            _options = options;
            _filename = filename;
        }

        internal Canvas(DocumentFiltersBase parent, System.IO.Stream stream, CanvasType type, string options)
        {
            _type = type;
            _options = options;
            _stream = new IGRStreamBridge(stream);
            _streamHandle = _stream.NewWritableStreamPtr();
        }
        internal Canvas(DocumentFiltersBase parent, IGRStream stream, CanvasType type, string options)
        {
            _type = type;
            _options = options;
            _stream = stream;
            _streamHandle = stream.NewWritableStreamPtr();
        }
        ~Canvas()
        {
            Dispose();
        }
        public virtual void Dispose()
        {
            lock (this)
            {
                Close();
            }
        }
        void IDisposable.Dispose()
        {
            Close();
        }
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
        public void Close()
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
        public void RenderPage(Page page)
        {
            VerifyArgumentNotNull(page, "page");
            Check(ISYS11df.IGR_Render_Page(page.Handle, Handle, ref ecb));
        }

        public void RenderPages(Extractor extractor)
        {
            VerifyArgumentNotNull(extractor, "extractor");

            foreach (var page in extractor.Pages) using (page)
            {
                RenderPage(page);
            }
        }
        public void AddCustomMetadata(string name, string value)
        {
            VerifyArgumentNotEmpty(name, "name");
            VerifyArgumentNotEmpty(value, "value");
            if (_handle != -1)
                throw new IGRException(13, "Metadata must be added before any drawing is attempt");
            _options += $"META(\"{name}\", \"{value}\");";
        }
        public void Arc(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4)
        {
            Check(ISYS11df.IGR_Canvas_Arc(Handle, x, y, x2, y2, x3, y3, x4, y4, ref ecb));
        }
        public void AngleArc(int x1, int y1, int radius, int startAngle, int sweepAngle)
        {
            Check(ISYS11df.IGR_Canvas_AngleArc(Handle, x1, y1, radius, startAngle, sweepAngle, ref ecb));
        }
        public void Chord(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4)
        {
            Check(ISYS11df.IGR_Canvas_Chord(Handle, x, y, x2, y2, x3, y3, x4, y4, ref ecb));
        }
        public void Ellipse(int x, int y, int x2, int y2)
        {
            Check(ISYS11df.IGR_Canvas_Ellipse(Handle, x, y, x2, y2, ref ecb));
        }
        public void Rect(int x, int y, int x2, int y2)
        {
            Check(ISYS11df.IGR_Canvas_Rect(Handle, x, y, x2, y2, ref ecb));
        }
        public void LineTo(int x, int y)
        {
            Check(ISYS11df.IGR_Canvas_LineTo(Handle, x, y, ref ecb));
        }
        public void MoveTo(int x, int y)
        {
            Check(ISYS11df.IGR_Canvas_MoveTo(Handle, x, y, ref ecb));
        }
        public void Pie(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4)
        {
            Check(ISYS11df.IGR_Canvas_Pie(Handle, x, y, x2, y2, x3, y3, x4, y4, ref ecb));
        }
        public void RoundRect(int x, int y, int x2, int y2, int radius)
        {
            Check(ISYS11df.IGR_Canvas_RoundRect(Handle, x, y, x2, y2, radius, ref ecb));
        }
        public void TextOut(int x, int y, string text)
        {
            Check(ISYS11df.IGR_Canvas_TextOut(Handle, x, y, text, ref ecb));
        }
        public void TextRect(int x, int y, int x2, int y2, string text, int flags)
        {
            Check(ISYS11df.IGR_Canvas_TextRect(Handle, x, y, x2, y2, text, flags, ref ecb));
        }
        public int TextWidth(string text)
        {
            int width = 0, height = 0;
            Check(ISYS11df.IGR_Canvas_MeasureText(Handle, text, ref width, ref height, ref ecb));
            return width;
        }
        public int TextHeight(string text)
        {
            int width = 0, height = 0;
            Check(ISYS11df.IGR_Canvas_MeasureText(Handle, text, ref width, ref height, ref ecb));
            return height;
        }
        public void SetPen(int color, int width, int style)
        {
            Check(ISYS11df.IGR_Canvas_SetPen(Handle, color, width, style, ref ecb));
        }
        public void SetBrush(int color, int style)
        {
            Check(ISYS11df.IGR_Canvas_SetBrush(Handle, color, style, ref ecb));
        }
        public void SetFont(string text, int size, int style)
        {
            Check(ISYS11df.IGR_Canvas_SetFont(Handle, text, size, style, ref ecb));
        }
        public void SetOpacity(int opacity)
        {
            Check(ISYS11df.IGR_Canvas_SetOpacity(Handle, (byte) opacity, ref ecb));
        }

        public void DrawImage(int x, int y, byte[] imagedata, string mimetype)
        {
            Check(ISYS11df.IGR_Canvas_DrawImage(Handle, x, y, imagedata, new IntPtr(imagedata.Length), mimetype, ref ecb));
        }
        public void DrawScaleImage(int x, int y, int x2, int y2, byte[] imagedata, string mimetype)
        {
            Check(ISYS11df.IGR_Canvas_DrawScaleImage(Handle, x, y, x2, y2, imagedata, new IntPtr(imagedata.Length), mimetype, ref ecb));
        }
        public void Rotation(int degrees)
        {
            Check(ISYS11df.IGR_Canvas_Rotation(Handle, degrees, ref ecb));
        }
        public void Reset()
        {
            Check(ISYS11df.IGR_Canvas_Reset(Handle, ref ecb));
        }
    }
}
