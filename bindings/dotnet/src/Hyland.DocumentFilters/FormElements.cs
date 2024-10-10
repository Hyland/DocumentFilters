//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// The FormElement interface represents a single form control for a given page.
    /// </summary>
    public class FormElement : DocumentFiltersBase, global::System.IDisposable
    {
        /// <summary>
        /// Represents a single option for a combo box or list box.
        /// </summary>
        public class OptionItem
        {
            /// <summary>
            /// Gets the name of the option.
            /// </summary>
            public string Name { get; internal set; }

            /// <summary>
            /// Gets the value of the option.
            /// </summary>
            public string Value { get; internal set; }

            /// <summary>
            /// Gets a value indicating whether the option is selected.
            /// </summary>
            public bool Selected { get; internal set; }
        }
        private IGR_Page_Form_Element _element;
        private OptionsCollection _options = new OptionsCollection();

        internal FormElement(IGR_Page_Form_Element element)
        {
            _element = element;

            for (var i = 0; i < _element.option_count; ++i)
            {
                _options.AddValue(new OptionItem
                {
                    Name = GetOptionStr(ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_GET_NAME, i),
                    Value = GetOptionStr(ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_GET_NAME, i),
                    Selected = GetOptionStr(ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_GET_SELECTED, i) == "1"
                });
            }
        }

        /// <summary>
        /// Represents the type of the form element.
        /// </summary>
        public enum ElementType
        {
            /// <summary>
            /// Indicates a button element.
            /// </summary>
            Button = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_TYPE_BUTTON,

            /// <summary>
            /// Indicates a radio button element.
            /// </summary>
            RadioButton = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_TYPE_RADIOBUTTON,

            /// <summary>
            /// Indicates a check box element.
            /// </summary>
            CheckBox = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_TYPE_CHECKBOX,

            /// <summary>
            /// Indicates a file field element.
            /// </summary>
            FileSelect = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_TYPE_FILESELECT,

            /// <summary>
            /// Indicates a multi-line text element.
            /// </summary>
            MultiLineText = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_TYPE_MULTILINE_TEXT,

            /// <summary>
            /// Indicates a single-line text element.
            /// </summary>
            SingleLineText = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_TYPE_SINGLELINE_TEXT,

            /// <summary>
            /// Indicates a combo box element.
            /// </summary>
            ComboBox = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_TYPE_COMBOBOX,

            /// <summary>
            /// Indicates a list box element.
            /// </summary>
            ListBox = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_TYPE_LISTBOX,

            /// <summary>
            /// Indicates a signature element.
            /// </summary>
            Signature = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_TYPE_SIGNATURE,

            /// <summary>
            /// Indicates a password element.
            /// </summary>
            Password = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_TYPE_PASSWORD,
        }

        /// <summary>
        /// Represents the flags for the form element.
        /// </summary>
        [System.Flags]
        public enum Flag
        {
            /// <summary>
            /// Indicates the form element is read-only.
            /// </summary>
            ReadOnly = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_FLAG_READONLY,

            /// <summary>
            /// Indicates the form element is required.
            /// </summary>
            Required = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_FLAG_REQUIRED,

            /// <summary>
            /// Indicates the form element is a comb text box.
            /// </summary>
            Comb = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_FLAG_COMB,

            /// <summary>
            /// Indicates the form element is checked.
            /// </summary>
            Checked = ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_FLAG_CHECKED
        }

        /// <summary>
        /// Gets the X coordinate of the form element.
        /// </summary>
        public int X => _element.x;

        /// <summary>
        /// Gets the Y coordinate of the form element.
        /// </summary>
        public int Y => _element.y;

        /// <summary>
        /// Gets the width of the form element.
        /// </summary>
        public int Width => _element.width;

        /// <summary>
        /// Gets the height of the form element.
        /// </summary>
        public int Height => _element.height;

        /// <summary>
        /// Gets the name of the form element.
        /// </summary>
        public string Name => _element.name;

        /// <summary>
        /// Gets the value of the form element.
        /// </summary>
        public string Value => GetStr(ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_GET_VALUE);

        /// <summary>
        /// Gets the caption of the form element.
        /// </summary>
        public string Caption => GetStr(ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_GET_CAPTION);

        /// <summary>
        /// Gets the font name of the form element.
        /// </summary>
        public string FontName => GetStr(ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_GET_FONT_NAME);

        /// <summary>
        /// Gets the action of the form element.
        /// </summary>
        public string Action => GetStr(ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_GET_ACTION);

        /// <summary>
        /// Gets the action destination of the form element.
        /// </summary>
        public string ActionDest => GetStr(ISYS11dfConstants.IGR_PAGE_FORM_ELEMENT_GET_ACTION_DEST);

        /// <summary>
        /// Gets the form element type.
        /// </summary>
        public ElementType Type => (ElementType)_element.type;

        /// <summary>
        /// Gets the flags of the form element.
        /// </summary>
        public Flag Flags => (Flag)_element.flags;

        /// <summary>
        /// Gets the font size of the form element.
        /// </summary>
        public int FontSize => _element.fontSize;

        /// <summary>
        /// Gets the rotation of the form element.
        /// </summary>
        public int Rotation => _element.rotation;

        /// <summary>
        /// Gets the index of the selected item.
        /// </summary>
        public int Selected => _element.selectedItem;

        /// <summary>
        /// Gets the check state of the form element.
        /// </summary>
        public bool Checked => (Flags & Flag.Checked) != 0;

        /// <summary>
        /// Gets the options of the form element.
        /// </summary>
        public ReadOnlyList<OptionItem> Options => _options;

        private string GetStr(int type, int maxLength = 4096)
        {
            Error_Control_Block ecb = new Error_Control_Block();
            StringBuilder res = new StringBuilder(maxLength);

            Check(ISYS11df.IGR_Get_Page_Form_Element_Str(ref _element, type, res.Capacity, res, ref ecb), ecb);
            return res.ToString();
        }
        private string GetOptionStr(int type, int index, int maxLength = 4096)
        {
            Error_Control_Block ecb = new Error_Control_Block();
            StringBuilder res = new StringBuilder(maxLength);

            Check(ISYS11df.IGR_Get_Page_Form_Element_Option_Str(ref _element, type, index, res.Capacity, res, ref ecb), ecb);
            return res.ToString();
        }

        /// <summary>
        /// Represents a collection of options for a combo box or list box.
        /// </summary>
        public class OptionsCollection : ReadOnlyList<OptionItem>
        {
            private List<OptionItem> _values = new List<OptionItem>();
            internal void AddValue(OptionItem item)
            {
                _values.Add(item);
            }

            /// <summary>
            /// Gets the number of options in the collection.
            /// </summary>
            protected override int GetCount()
            {
                return _values.Count;
            }

            /// <summary>
            /// Gets the option at the specified index.
            /// </summary>
            protected override OptionItem GetItem(int index)
            {
                return _values[index];
            }
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        /// <summary>
        /// Disposes of the form element.
        /// </summary>
        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects).
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.

                disposedValue = true;
            }
        }

        /// <summary>
        /// Disposes of the form element.
        /// </summary>
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            // GC.SuppressFinalize(this);
        }
        #endregion
    }
}

