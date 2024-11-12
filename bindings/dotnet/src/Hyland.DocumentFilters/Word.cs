//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// Represents a word extracted from a page.
    /// </summary>
    public class Word : global::System.IDisposable
    {
        private IGR_Page_Word _word;
        private int _index;

        internal Word(IGR_Page_Word word, int index)
        {
            _word = word;
            _index = index;
        }

        /// <summary>
        /// Dispose of the word.
        /// </summary>
        ~Word()
        {
            Dispose();
        }

        /// <summary>
        /// Dispose of the word.
        /// </summary>
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

        /// <summary>
        /// Returns the X position of the word.
        /// </summary>
        public int X => GetX();

        /// <summary>
        /// Returns the Y position of the word.
        /// </summary>
        public int Y => GetY();

        /// <summary>
        /// Returns the width of the word.
        /// </summary>
        public int Width => GetWidth();

        /// <summary>
        /// Returns the height of the word.
        /// </summary>
        public int Height => GetHeight();

        /// <summary>
        /// Returns the character offset of the word.
        /// </summary>
        public int CharacterOffset => GetCharacterOffset();

        /// <summary>
        /// Returns the text of the word.
        /// </summary>
        public string Text => GetText();


        /// <summary>
        /// Returns the index of the word on the current page.
        /// </summary>
        public int WordIndex => GetWordIndex();
    }
}
