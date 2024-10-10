using System;
using System.Collections.Generic;
using System.Text;

namespace Hyland.DocumentFilters
{
#if !NETSTANDARD
    using StyleDictionary = IDictionary<string, string>;
#else
    using StyleDictionary = IReadOnlyDictionary<string, string>;
#endif

    /// <summary>
    /// Represents the type of a page element.
    /// </summary>
    public enum PageElementType
    {
        /// <summary>
        /// Indicates a page element of unknown page.
        /// </summary>
        Page = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_PAGE,

        /// <summary>
        /// Indicates a section page element.
        /// </summary>
        Section = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_SECTION,

        /// <summary>
        /// Indicates a column page element.
        /// </summary>
        Column = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_COLUMN,

        /// <summary>
        /// Indicates a paragraph page element.
        /// </summary>
        Paragraph = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_PARAGRAPH,

        /// <summary>
        /// Indicates a line page element.
        /// </summary>
        Line = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_LINE,

        /// <summary>
        /// Indicates a run page element.
        /// </summary>
        Run = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_RUN,

        /// <summary>
        /// Indicates a word page element.
        /// </summary>
        Word = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_WORD,

        /// <summary>
        /// Indicates a character page element.
        /// </summary>
        Header = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_HEADER,

        /// <summary>
        /// Indicates a footer page element.
        /// </summary>
        Footer = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_FOOTER,

        /// <summary>
        /// Indicates a image page element.
        /// </summary>
        Image = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_IMAGE,

        /// <summary>
        /// Initiates a table page element.
        /// </summary>
        Table = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_TABLE,

        /// <summary>
        /// Initiates a table row page element.
        /// </summary>
        TableRow = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_TABLE_ROW,

        /// <summary>
        /// Initiates a table cell page element.
        /// </summary>
        TableCell = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_TABLE_CELL,

        /// <summary>
        /// Initiates a form page element.
        /// </summary>
        Form = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_FORM,

        /// <summary>
        /// Initiates a form element page element.
        /// </summary>
        FormElement = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_FORM_ELEMENT,

        /// <summary>
        /// Initiates a floating page element.
        /// </summary>
        Float = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_FLOAT,

        /// <summary>
        /// Initiates a graphic page element.
        /// </summary>
        Graphic = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_GRAPHIC,

        /// <summary>
        /// Initiates a text box page element.
        /// </summary>
        TextBox = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_TEXT_BOX,

        /// <summary>
        /// Initiates a note page element.
        /// </summary>
        Notes = ISYS11dfConstants.IGR_PAGE_ELEMENT_TYPE_NOTES,
    }

    /// <summary>
    /// Represents a page element in a document.
    /// </summary>
    public class PageElement : DocumentFiltersBase
    {
        private readonly IntPtr _pageHandle;
        private IGR_Page_Element _info;
        private string _text = null;
        private Dictionary<string, string> _styles = null;

        internal PageElement(IntPtr pageHandle, IGR_Page_Element info)
        {
            _pageHandle = pageHandle;
            _info = info;
        }

        /// <summary>
        /// Gets the bounds of the page element.
        /// </summary>
        public System.Drawing.RectangleF Bounds => System.Drawing.RectangleF.FromLTRB(_info.pos.left, _info.pos.top, _info.pos.right, _info.pos.bottom);

        /// <summary>
        /// Gets the depth of the page element.
        /// </summary>
        public int Depth => (int)_info.depth;


        /// <summary>
        /// Gets the flags of the page element.
        /// </summary>
        public int Flags => (int)_info.flags;

        /// <summary>
        /// Gets the type of the page element.
        /// </summary>
        public PageElementType Type => (PageElementType)_info.type;

        /// <summary>
        /// Gets the first child of the page element.
        /// </summary>
        /// <returns>PageElement of the first child, or null if no children present.</returns>
        public PageElement GetFirstChild()
        {
            IGR_Page_Element res = new IGR_Page_Element();
            res.struct_size = (uint)Marshaler.SizeOf<IGR_Page_Element>();

            if (ISYS11df.IGR_Get_Page_Element_First_Child(_pageHandle, ref _info, ref res, ref ecb) == 0)
                return new PageElement(_pageHandle, res);
            else
                return null;
        }

        /// <summary>
        /// Gets the next sibling of the page element.
        /// </summary>
        /// <returns>PageElement of the next sibling child, or null if last item.</returns>
        public PageElement GetNextSibling()
        {
            IGR_Page_Element res = new IGR_Page_Element();
            res.struct_size = (uint)Marshaler.SizeOf<IGR_Page_Element>();

            if (ISYS11df.IGR_Get_Page_Element_Next_Sibling(_pageHandle, ref _info, ref res, ref ecb) == 0)
                return new PageElement(_pageHandle, res);
            else
                return null;
        }

        /// <summary>
        /// Get the named style from the page element.
        /// </summary>
        /// <param name="styleName">The name of the style</param>
        /// <returns>String containing the name, or null if not available.</returns>
        public string GetStyle(string styleName)
        {
            StringBuilder res = new StringBuilder(4096);

            uint len = (uint)res.Capacity;
            if (ISYS11df.IGR_Get_Page_Element_Style(_pageHandle, ref _info, styleName, ref len, res, ref ecb) == 0)
                return res.ToString();
            else
                return null;
        }

        /// <summary>
        /// Gets the text of the page element.
        /// </summary>
        public string Text
        {
            get
            {
                if (_text == null)
                {
                    uint len = 0;
                    if (ISYS11df.IGR_Get_Page_Element_Text(_pageHandle, ref _info, ref len, null, ref ecb) == 0)
                    {
                        ++len; // space for null
                        StringBuilder res = new StringBuilder((int)len);
                        if (ISYS11df.IGR_Get_Page_Element_Text(_pageHandle, ref _info, ref len, res, ref ecb) == 0)
                            _text = res.ToString();
                    }
                }
                return _text;
            }
        }

        /// <summary>
        /// Gets the styles of the page element.
        /// </summary>
        public StyleDictionary Styles
        {
            get
            {
                if (_styles == null)
                {
                    _styles = new Dictionary<string, string>();

                    Error_Control_Block ecb = new Error_Control_Block();
                    Check(ISYS11df.IGR_Get_Page_Element_Styles(_pageHandle, ref _info, (name, value, _) =>
                    {
                        _styles[name] = value;
                        return 0;
                    }, IntPtr.Zero, ref ecb), ecb);
                }

                return _styles;
            }
        }

        /// <summary>
        /// Gets the children of the page element.
        /// </summary>
        public IEnumerable<PageElement> Children()
        {
            PageElement child = GetFirstChild();
            while (child != null)
            {
                yield return child;
                child = child.GetNextSibling();
            }
        }

        /// <summary>
        /// Gets the children, including all descendants, of the page element.
        /// </summary>
        public IEnumerable<PageElement> AllChildren()
        {
            PageElement child = GetFirstChild();
            while (child != null)
            {
                yield return child;
                foreach (var grandChild in child.AllChildren())
                    yield return grandChild;
                child = child.GetNextSibling();
            }
        }

        /// <summary>
        /// ToString override.
        /// </summary>
        public override string ToString() => $"{Type} {Bounds} {Text}";
    }
}
