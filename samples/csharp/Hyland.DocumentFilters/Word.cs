//===========================================================================
// (c) 2018 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

namespace Hyland.DocumentFilters
{

    public class Word : global::System.IDisposable
    {
        private IGR_Page_Word _word;
        private int _index;

        internal Word(IGR_Page_Word word, int index)
        {
            _word = word;
            _index = index;
        }

        ~Word()
        {
            Dispose();
        }

        public virtual void Dispose()
        {
        }

        /// <summary>
        /// The Text property returns a Unicode string for the text of this word.
        /// </summary>
        /// <returns></returns>
        public string GetText()
        {
            return _word.word;
        }

        /// <summary>
        /// The X and Y properties return the position of the word in pixels. The position information is based on the DPI used when loading the page.
        /// </summary>
        /// <returns></returns>
        public int GetX()
        {
            return _word.x;
        }

        /// <summary>
        /// The X and Y properties return the position of the word in pixels. The position information is based on the DPI used when loading the page.
        /// </summary>
        /// <returns></returns>
        public int GetY()
        {
            return _word.y;
        }

        /// <summary>
        /// The Width and Height properties return the dimensions of the word in pixels. The dimension information is based on the DPI used when loading the page.
        /// </summary>
        /// <returns></returns>
        public int GetWidth()
        {
            return _word.width;
        }

        /// <summary>
        /// The Width and Height properties return the dimensions of the word in pixels. The dimension information is based on the DPI used when loading the page.
        /// </summary>
        /// <returns></returns>
        public int GetHeight()
        {
            return _word.height;
        }

        /// <summary>
        /// The CharacterOffset property contains the character offset of the word into the text on the current page. 
        /// </summary>
        /// <returns></returns>
        public int GetCharacterOffset()
        {
            return _word.charoffset;
        }

        /// <summary>
        /// Return the index of the word on the current page. 
        /// </summary>
        /// <returns></returns>
        public int GetWordIndex()
        {
            return _index;
        }

        public int X => GetX();

        public int Y => GetY();

        public int Width => GetWidth();

        public int Height => GetHeight();

        public int CharacterOffset => GetCharacterOffset();

        public string Text => GetText();

        public int WordIndex => GetWordIndex();
    }
}
