//===========================================================================
// (c) 2018 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace Hyland.DocumentFilters
{

    public class Page : DocumentFiltersBase, global::System.IDisposable
    {
        private int _docHandle;
        private int _pageHandle;
        private IGR_Page_Word[] _words;
        private int _wordIndex = 0;
        private string _pageText = "";

        public int WordCount => GetWordCount();
        public int Width => GetWidth();
        public int Height => GetHeight();
        public string Text => GetText();
        public Word FirstWord => GetFirstWord();
        public Word NextWord => GetNextWord();
        public ReadOnlyList<Word> Words => new WordCollection(this);
        public int Handle { get { if (_pageHandle < 0) throw new IGRException(4, "Page has been closed"); return _pageHandle; } }
        public IEnumerable<SubFile> Images => new Extractor.SubFileCollection(this, () => this.GetFirstImage(), () => this.GetNextImage());

        internal Page(int docHandle, int pageHandle)
        {
            _docHandle = docHandle;
            _pageHandle = pageHandle;
        }
        ~Page()
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
        /// IGR_Get_Page_Attribute returns style or properties of an open page; see <page> under Structured XML for a full list of options.
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
    
    }
}

