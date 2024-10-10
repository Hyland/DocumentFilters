//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing.Printing;
using System.Runtime.InteropServices;
using System.Text;

using Hyland.DocumentFilters.Annotations;

namespace Hyland.DocumentFilters
{

    /// <summary>
    /// The Page interface represents a single page in an image laid-out document. The page allows access to the words 
    /// on a page, as well as the ability to render it onto a canvas such as TIFF, PNG, or PDF.
    /// </summary>
    public class Page : DocumentFiltersBase, global::System.IDisposable
    {
        private int _docHandle;
        private int _pageHandle;
        private IGR_Page_Word[] _words;
        private List<FormElement> _formElements;
        private List<Hyperlink> _hyperlinks;
        private List<Annotation> _annotations;
        private int _wordIndex = 0;
        private string _pageText = "";

        /// <summary>
        /// Gets the number of words on the page.
        /// </summary>
        public int WordCount => GetWordCount();

        /// <summary>
        /// Gets the width of the page in pixels.
        /// </summary>
        public int Width => GetWidth();

        /// <summary>
        /// Gets the height of the page in pixels.
        /// </summary>
        public int Height => GetHeight();

        /// <summary>
        /// Gets the text of the page.
        /// </summary>
        public string Text => GetText();

        /// <summary>
        /// Gets the first word on the page.
        /// </summary>
        public Word FirstWord => GetFirstWord();

        /// <summary>
        /// Gets the next word on the page.
        /// </summary>
        public Word NextWord => GetNextWord();

        /// <summary>
        /// Gets the list of words on the page.
        /// </summary>
        public ReadOnlyList<Word> Words => new WordCollection(this);

        /// <summary>
        /// Gets the IGR_HPAGE handle for the page.
        /// </summary>
        public int Handle { get { if (_pageHandle < 0) throw new IGRException(4, "Page has been closed"); return _pageHandle; } }

        /// <summary>
        /// Gets the images on the page.
        /// </summary>
        public IEnumerable<SubFile> Images => CreateImageEnumeration();

        /// <summary>
        /// Gets the form elements on the page.
        /// </summary>
        public FormElementCollection FormElements => new FormElementCollection(NeedFormElements());

        /// <summary>
        /// Gets the hyperlinks on the page.
        /// </summary>
        public HyperlinkCollection Hyperlinks => new HyperlinkCollection(NeedHyperlinks());

        /// <summary>
        /// Gets the annotations on the page.
        /// </summary>
        public AnnotationCollection Annotations => new AnnotationCollection(NeedAnnotations());

        /// <summary>
        /// Internal constructor
        /// </summary>
        internal Page(int docHandle, int pageHandle)
        {
            _docHandle = docHandle;
            _pageHandle = pageHandle;
        }

        /// <summary>
        /// Dispose of the page.
        /// </summary>
        ~Page()
        {
            Dispose();
        }

        /// <summary>
        /// Dispose of the page.
        /// </summary>
        public virtual void Dispose()
        {
            lock (this)
            {
                Close();
            }
        }

        /// <summary>
        /// Dispose of the page.
        /// </summary>
        void global::System.IDisposable.Dispose()
        {
            Close();
        }

        /// <summary>
        /// The Close method releases any resources associated with the page. 
        /// </summary>
        public void Close()
        {
            if (_pageHandle > 0)
            {
                Error_Control_Block ecb = new Error_Control_Block();
                Check(ISYS11df.IGR_Close_Page(_pageHandle, ref ecb), ecb);
                _pageHandle = 0;
            }
        }

        /// <summary>
        /// The WordCount property returns the number of “Word”s that are on a page. The words can be enumerated using the GetFirstWord and GetNextWord methods. 
        /// </summary>
        /// <returns></returns>
        public int GetWordCount()
        {
            Error_Control_Block ecb = new Error_Control_Block();
            int retval = 0;
            Check(ISYS11df.IGR_Get_Page_Word_Count(Handle, ref retval, ref ecb), ecb);
            return retval;
        }

        /// <summary>
        /// Returns the word at the given index
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        public Word GetWord(int index)
        {
            NeedWords();
            return new Word(_words[index], index);
        }

        /// <summary>
        /// The width and height properties return the dimensions of a page in pixels.
        /// </summary>
        /// <returns></returns>
        public int GetWidth()
        {
            Error_Control_Block ecb = new Error_Control_Block();
            int width = 0, height = 0;
            Check(ISYS11df.IGR_Get_Page_Dimensions(Handle, ref width, ref height, ref ecb), ecb);
            return width;
        }

        /// <summary>
        /// The width and height properties return the dimensions of a page in pixels.
        /// </summary>
        /// <returns></returns>
        public int GetHeight()
        {
            Error_Control_Block ecb = new Error_Control_Block();
            int width = 0, height = 0;
            Check(ISYS11df.IGR_Get_Page_Dimensions(Handle, ref width, ref height, ref ecb), ecb);
            return height;
        }

        /// <summary>
        /// The Text property returns all the text contained on the page. 
        /// </summary>
        /// <returns></returns>
        public string GetText()
        {
            if (string.IsNullOrEmpty(_pageText))
            {
                Error_Control_Block ecb = new Error_Control_Block();
                StringBuilder buffer = new StringBuilder(4096);
                int bufferSize = buffer.Capacity;
                
                while (ISYS11df.IGR_Get_Page_Text(Handle, buffer, ref bufferSize, ref ecb) == 0)
                {
                    _pageText += buffer.ToString(0, bufferSize);
                }
            }
            return _pageText;
        }

        /// <summary>
        /// The FirstWord and NextWord properties enumerate all the words on the current page. FirstWord resets the enumeration back to the beginning. The property will return a reference to a Word object, or NULL if there are no more words. 
        /// </summary>
        /// <returns></returns>
        public Word GetFirstWord()
        {
            _wordIndex = 0;
            return GetNextWord();
        }

        /// <summary>
        /// The FirstWord and NextWord properties enumerate all the words on the current page. FirstWord resets the enumeration back to the beginning. The property will return a reference to a Word object, or NULL if there are no more words. 
        /// </summary>
        /// <returns></returns>
        public Word GetNextWord()
        {
            NeedWords();
            if (_wordIndex < _words.Length)
            {
                var retval = new Word(_words[_wordIndex], _wordIndex);
                _wordIndex++;
                return retval;
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// The FirstImage and NextImage enumerate the embedded images that are on the page. This method is useful if the page images are to be extracted and stored in separate files. These properties are not needed if the page is to be rendered into an image output canvas such as PNG, TIFF, or PDF.
        /// </summary>
        /// <returns></returns>
        public SubFile GetFirstImage()
        {
            return GetNextImage();
        }

        /// <summary>
        /// The FirstImage and NextImage enumerate the embedded images that are on the page. This method is useful if the page images are to be extracted and stored in separate files. These properties are not needed if the page is to be rendered into an image output canvas such as PNG, TIFF, or PDF.
        /// </summary>
        /// <returns></returns>
        public SubFile GetNextImage()
        {
            StringBuilder id = new StringBuilder(4096);
            StringBuilder name = new StringBuilder(1024);
            Error_Control_Block ecb = new Error_Control_Block();
            long date = 0;
            long size = 0;

            if (ISYS11df.IGR_Get_Page_Image_Entry(Handle, id, name, ref date, ref size, ref ecb) == 0)
            {
                return new SubFile(this, Handle, id.ToString(), name.ToString(), size, date, ISYS11df.IGR_Extract_Page_Image_Stream);
            }
            return null;
        }

        /// <summary>
        /// Redact removes a range of words and blacks out the location for the specified range from the page. 
        /// </summary>
        /// <param name="firstWord"></param>
        /// <param name="lastWord"></param>
        public void Redact(Word firstWord, Word lastWord)
        {
            Redact(firstWord.WordIndex, lastWord.WordIndex);
        }

        /// <summary>
        /// Redact removes a range of words and blacks out the location for the specified range from the page. 
        /// </summary>
        /// <param name="firstWord"></param>
        /// <param name="lastWord"></param>
        public void Redact(int firstWord, int lastWord)
        {
            Error_Control_Block ecb = new Error_Control_Block();

            Check(ISYS11df.IGR_Redact_Page_Text(_pageHandle, firstWord, lastWord, 0, ref ecb), ecb);
        }

        /// <summary>
        /// IGR_Get_Page_Attribute returns style or properties of an open page; see &lt;page&gt; under Structured XML for a full list of options.
        /// </summary>
        /// <param name="Name"></param>
        /// <returns></returns>
        public string GetAttribute(string Name)
        {
            StringBuilder buffer = new StringBuilder(1024);
            int bufferSize = buffer.Capacity;
            Error_Control_Block ecb = new Error_Control_Block();

            Check(ISYS11df.IGR_Get_Page_Attribute(Handle, Name, buffer, ref bufferSize, ref ecb), ecb);
            return buffer.ToString(0, bufferSize);
        }

        /// <summary>
        /// Returns the page as a bitmap.
        /// </summary>
        /// <returns>System.Drawing.Bitmap containing the page image.</returns>
        public System.Drawing.Bitmap GetPageBitmap()
            => GetPageBitmap(System.Drawing.Rectangle.FromLTRB(0, 0, Width, Height), new System.Drawing.Size(Width, Height));

        /// <summary>
        /// Returns the page as a bitmap.
        /// </summary>
        /// <param name="output">The size of the image to generate.</param>
        /// <returns>System.Drawing.Bitmap containing the page image.</returns>
        public System.Drawing.Bitmap GetPageBitmap(System.Drawing.Size output)
            => GetPageBitmap(System.Drawing.Rectangle.FromLTRB(0, 0, Width, Height), output);

        /// <summary>
        /// Returns the page as a bitmap.
        /// </summary>
        /// <param name="input"></param>
        /// <returns>System.Drawing.Bitmap containing the page image.</returns>
        public System.Drawing.Bitmap GetPageBitmap(System.Drawing.Rectangle input)
            => GetPageBitmap(input, new System.Drawing.Size(input.Width, input.Height));

#if !NETSTANDARD
        [System.Runtime.InteropServices.DllImport("kernel32.dll", EntryPoint = "CopyMemory", SetLastError = false)]

        private static extern void CopyMemory(IntPtr dest, IntPtr src, uint count);
#endif

        /// <summary>
        /// Returns the page as a bitmap.
        /// </summary>
        /// <param name="source">The source rect of the area to copy.</param>
        /// <param name="output">The size of the output to create.</param>
        /// <param name="options">The canvas options to use when rendering the page.</param>
        /// <returns>System.Drawing.Bitmap containing the page image.</returns>
        public System.Drawing.Bitmap GetPageBitmap(System.Drawing.Rectangle source, System.Drawing.Size output, string options = "")
        {
            System.Drawing.Color MakeColor(int c)
            {
                return System.Drawing.Color.FromArgb(
                    (c >> 24) & 0xff,
                    (c >> 16) & 0xff,
                    (c >> 8) & 0xff,
                    (c >> 0) & 0xff);
            }

            IGR_Page_Pixels pixels = new IGR_Page_Pixels();
            IGR_Rect sourceRect = new IGR_Rect(source);
            IGR_Size destSize = new IGR_Size(output);

            Check(ISYS11df.IGR_Get_Page_Pixels(Handle, ref sourceRect, ref destSize, 0, options, ISYS11dfConstants.IGR_OPEN_BITMAP_PIXEL_AUTO, ref pixels, ref ecb), ecb);
            try
            {
                System.Drawing.Imaging.PixelFormat pixelFormat = MapPixelFormat(pixels.pixel_format);

                var res = new System.Drawing.Bitmap((int)pixels.width, (int)pixels.height, pixelFormat);
                if (pixels.palette_count > 0)
                {
                    System.Drawing.Imaging.ColorPalette palette = res.Palette;
                    for (var i = 0; i < pixels.palette_count; ++i)
                    {
                        if (i < palette.Entries.Length)
                            palette.Entries[i] = MakeColor(pixels.palette[i]);
                    }
                    res.Palette = palette;
                }

                var pix = res.LockBits(System.Drawing.Rectangle.FromLTRB(0, 0, res.Width, res.Height), System.Drawing.Imaging.ImageLockMode.ReadWrite, pixelFormat);
                try
                {
                    for (var y = 0; y < pixels.height; ++y)
                    {
#if !NETSTANDARD
                        IntPtr src = new IntPtr(pixels.scanline0.ToInt64() + (y * pixels.stride));
                        IntPtr dest = new IntPtr(pix.Scan0.ToInt64() + (y * pix.Stride));
                        
                        CopyMemory(dest, src, (uint) Math.Abs(pix.Stride));
#else
                        unsafe
                        {
                            byte* src = (byte*) pixels.scanline0.ToPointer() + (y * pixels.stride);
                            byte* dest = (byte*) pix.Scan0.ToPointer() + (y * pix.Stride);

                            Buffer.MemoryCopy(src, dest, Math.Abs(pix.Stride), Math.Abs(pix.Stride));
                        }
#endif

                    }
                }
                finally
                {
                    res.UnlockBits(pix);
                }

                return res;
            }
            finally
            {
                Check(ISYS11df.IGR_Free_Page_Pixels(ref pixels, ref ecb), ecb);
            }
        }

        /// <summary>
        /// Compare this page to another page.
        /// </summary>
        /// <param name="otherPage">The other page to compare to.</param>
        /// <param name="settings">The settings for the comparison.</param>
        /// <returns>CompareResults containing the results of the comparison.</returns>
        public CompareResults Compare(Page otherPage, CompareSettings settings = null)
            => Compare(otherPage, null, null, settings);

        /// <summary>
        /// Compare this page to another page.
        /// </summary>
        /// <param name="otherPage">The other page to compare to.</param>
        /// <param name="margins">The top, left, right and bottom margins.</param>
        /// <param name="settings">The settings for the comparison.</param>
        /// <returns>CompareResults containing the results of the comparison.</returns>
        public CompareResults Compare(Page otherPage, System.Drawing.RectangleF margins, CompareSettings settings = null)
            => Compare(otherPage, margins, margins, settings);

        /// <summary>
        /// Compare this page to another page.
        /// </summary>
        /// <param name="otherPage">The other page to compare to.</param>
        /// <param name="leftMargins">The top, left, right and bottom margins for the left page.</param>
        /// <param name="rightMargins">The top, left, right and bottom margins for the right page.</param>
        /// <param name="settings">The settings for the comparison.</param>
        /// <returns>CompareResults containing the results of the comparison.</returns>
        public CompareResults Compare(Page otherPage, System.Drawing.RectangleF? leftMargins, System.Drawing.RectangleF? rightMargins, CompareSettings settings = null)
        {
            IntPtr enumerator = IntPtr.Zero;
            IGR_Text_Compare_Settings wrappedSettings = (settings ?? new CompareSettings()).Make();

            IntPtr wrappedLeftMargins = leftMargins != null ? leftMargins.ToIgrRectF().ToIntPtr() : IntPtr.Zero;
            IntPtr wrappedRightMargins = rightMargins != null ? rightMargins.ToIgrRectF().ToIntPtr() : IntPtr.Zero;
            try
            {

                Check(ISYS11df.IGR_Text_Compare_Pages(
                    this.Handle,
                    wrappedLeftMargins,
                    otherPage.Handle,
                    wrappedRightMargins,
                    ref wrappedSettings,
                    out enumerator,
                    ref ecb), ecb);
                return CompareResults.Make(enumerator);
            } 
            finally
            {
                if (wrappedLeftMargins != IntPtr.Zero)
                    Marshal.FreeHGlobal(wrappedLeftMargins);
                if (wrappedRightMargins != IntPtr.Zero)
                    Marshal.FreeHGlobal(wrappedRightMargins);
            }
        }

        /// <summary>
        /// Returns the root page element for this page.
        /// </summary>
        /// <returns>PageElement representing the root of the page.</returns>
        public PageElement GetRootPageElement()
        {
            Error_Control_Block ecb = new Error_Control_Block();
            IGR_Page_Element res = new IGR_Page_Element();
            res.struct_size = (uint)Marshaler.SizeOf<IGR_Page_Element>();

            if (ISYS11df.IGR_Get_Page_Element_Root((IntPtr) _pageHandle, ref res, ref ecb) == 0)
                return new PageElement((IntPtr)_pageHandle, res);
            else
                return null;
        }

        private System.Drawing.Imaging.PixelFormat MapPixelFormat(int value)
        {
            switch (value)
            {
                case ISYS11dfConstants.IGR_OPEN_BITMAP_PIXEL_1BPP_INDEXED: return System.Drawing.Imaging.PixelFormat.Format1bppIndexed;
                case ISYS11dfConstants.IGR_OPEN_BITMAP_PIXEL_4BPP_INDEXED: return System.Drawing.Imaging.PixelFormat.Format4bppIndexed;
                case ISYS11dfConstants.IGR_OPEN_BITMAP_PIXEL_8BPP_INDEXED: return System.Drawing.Imaging.PixelFormat.Format8bppIndexed;
                case ISYS11dfConstants.IGR_OPEN_BITMAP_PIXEL_16BPP_565_RGB: return System.Drawing.Imaging.PixelFormat.Format16bppRgb565;
                case ISYS11dfConstants.IGR_OPEN_BITMAP_PIXEL_24BPP_888_RGB: return System.Drawing.Imaging.PixelFormat.Format24bppRgb;
                case ISYS11dfConstants.IGR_OPEN_BITMAP_PIXEL_24BPP_888_BGR: return System.Drawing.Imaging.PixelFormat.Format24bppRgb;
                case ISYS11dfConstants.IGR_OPEN_BITMAP_PIXEL_32BPP_8888_ARGB: return System.Drawing.Imaging.PixelFormat.Format32bppRgb;
                case ISYS11dfConstants.IGR_OPEN_BITMAP_PIXEL_32BPP_8888_BGRA: return System.Drawing.Imaging.PixelFormat.Format32bppRgb;
                case ISYS11dfConstants.IGR_OPEN_BITMAP_PIXEL_32BPP_8888_RGBA: return System.Drawing.Imaging.PixelFormat.Format32bppRgb;
                case ISYS11dfConstants.IGR_OPEN_BITMAP_PIXEL_32BPP_8888_ABGR: return System.Drawing.Imaging.PixelFormat.Format32bppRgb;
                default: throw new IGRException(4, "Cannot decode pixel format");
            }
        }

        private void NeedWords()
        {
            if (_words == null)
            {
                Error_Control_Block ecb = new Error_Control_Block();
                int wordCount = GetWordCount();
                _words = new IGR_Page_Word[wordCount];
                if (wordCount > 0)
                {
                    Check(ISYS11df.IGR_Get_Page_Words(_pageHandle, 0, ref wordCount, _words, ref ecb), ecb);
                }
            }
        }

        private List<FormElement> NeedFormElements()
        {
            if (_formElements == null)
            {
                Error_Control_Block ecb = new Error_Control_Block();
                _formElements = new List<FormElement>();

                int count = 0;
                Check(ISYS11df.IGR_Get_Page_Form_Element_Count(_pageHandle, ref count, ref ecb), ecb);
                for (int i = 0; i < count; ++i)
                {
                    int read = 1;
                    IGR_Page_Form_Element item = new IGR_Page_Form_Element();
                    Check(ISYS11df.IGR_Get_Page_Form_Elements(_pageHandle, i, ref read, ref item, ref ecb), ecb);
                    if (read == 1)
                        _formElements.Add(new FormElement(item));
                }
            }
            return _formElements;
        }

        private List<Hyperlink> NeedHyperlinks()
        {
            if (_hyperlinks == null)
            {
                Error_Control_Block ecb = new Error_Control_Block();
                _hyperlinks = new List<Hyperlink>();

                int count = 0;
                Check(ISYS11df.IGR_Get_Page_Hyperlink_Count(_pageHandle, ref count, ref ecb), ecb);
                for (int i = 0; i < count; ++i)
                {
                    int read = 1;
                    IGR_Hyperlink item = new IGR_Hyperlink();
                    Check(ISYS11df.IGR_Get_Page_Hyperlinks(_pageHandle, i, ref read, ref item, ref ecb), ecb);
                    if (read == 1)
                        _hyperlinks.Add(new Hyperlink(item));
                }
            }
            return _hyperlinks;
        }

        private List<Annotation> NeedAnnotations()
        {
            if (_annotations == null)
            {
                Error_Control_Block ecb = new Error_Control_Block();
                _annotations = new List<Annotation>();

                int count = 0;
                Check(ISYS11df.IGR_Get_Page_Annotation_Count(_pageHandle, ref count, ref ecb), ecb);
                for (int i = 0; i < count; ++i)
                {
                    int read = 1;
                    IGR_Annotation item = new IGR_Annotation();
                    Check(ISYS11df.IGR_Get_Page_Annotations(_pageHandle, i, ref read, ref item, ref ecb), ecb);
                    if (read == 1)
                        _annotations.Add(Annotation.From(item));
                }
            }
            return _annotations;
        }

        private IEnumerable<SubFile> CreateImageEnumeration()
        {
            return new Extractor.SubFileEnumeratorCollection(this, Handle, () => CreateImageEnumerator(), ISYS11df.IGR_Extract_Page_Image_Stream, GetFirstImage, GetNextImage);
        }

        private IntPtr CreateImageEnumerator()
        {
            IntPtr result = IntPtr.Zero;
            Error_Control_Block ecb = new Error_Control_Block();
            Check(ISYS11df.IGR_Get_Page_Images_Enumerator(_pageHandle, ref result, ref ecb), ecb);
            return result;
        }


        internal class WordCollection : ReadOnlyList<Word>
        {
            private Page _page;

            internal WordCollection(Page page)
            {
                _page = page;
            }

            protected override int GetCount()
            {
                return _page.WordCount;
            }

            protected override Word GetItem(int index)
            {
                VerifyArgumentInRange(index, 0, GetCount(), "index");
                return _page.GetWord(index);
            }
        }

        /// <summary>
        /// Represents a list of page form elements.
        /// </summary>
        public class FormElementCollection : ReadOnlyList<FormElement>
        {
            private List<FormElement> _items;

            /// <summary>
            /// Returns the form element with the given name.
            /// </summary>
            public FormElement this[string name]
            {
                get
                { 
                    foreach (var i in _items)
                    {
                        if (i.Name == name)
                            return i;
                    }
                    return null;
                }
            }
            internal FormElementCollection(List<FormElement> items)
            {
                _items = items ?? new List<FormElement>();
            }

            /// <summary>
            /// Returns the number of form elements.
            /// </summary>
            protected override int GetCount()
            {
                return _items.Count;
            }

            /// <summary>
            /// Returns the form element at the given index.
            /// </summary>
            protected override FormElement GetItem(int index)
            {
                return _items[index];
            }
        }

        /// <summary>
        /// Represents a list of hyperlinks on a page.
        /// </summary>
        public class HyperlinkCollection : ReadOnlyList<Hyperlink>
        {
            private List<Hyperlink> _items;

            internal HyperlinkCollection(List<Hyperlink> items)
            {
                _items = items ?? new List<Hyperlink>();
            }

            /// <summary>
            /// Returns the number of hyperlinks.
            /// </summary>
            protected override int GetCount()
            {
                return _items.Count;
            }

            /// <summary>
            /// Returns the hyperlink at the given index.
            /// </summary>
            protected override Hyperlink GetItem(int index)
            {
                return _items[index];
            }
        }

        /// <summary>
        /// Represents a list of annotations on a page.
        /// </summary>
        public class AnnotationCollection : ReadOnlyList<Annotation>
        {
            private List<Annotation> _items;

            internal AnnotationCollection(List<Annotation> items)
            {
                _items = items ?? new List<Annotation>();
            }

            /// <summary>
            /// Returns the number of annotations.
            /// </summary>
            protected override int GetCount()
            {
                return _items.Count;
            }

            /// <summary>
            /// Returns the annotation at the given index.
            /// </summary>
            protected override Annotation GetItem(int index)
            {
                return _items[index];
            }
        }
    }
}

