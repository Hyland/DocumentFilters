//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Drawing;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// Represents the properties provided when rendering a page.
    /// </summary>
    public class RenderPageProperties
    {
        /// <summary>
        /// Contains the list of form values to use when rendering the page.
        /// </summary>
        public readonly List<IGR_Render_Page_Form_Values> FormValues = new List<IGR_Render_Page_Form_Values>();

        /// <summary>
        /// Contains the list of redactions to apply when rendering the page.
        /// </summary>
        public readonly List<IGR_Render_Page_Redactions> Redactions = new List<IGR_Render_Page_Redactions>();

        /// <summary>
        /// Get or set redaction flags.
        /// </summary>
        public int RedactionFlags { get; set; }

        private IEnumerator<IGR_Render_Page_Form_Values> _formValuesEnumerator;

        /// <summary>
        /// Get or set the source rectangle when copying part of the page.
        /// </summary>
        public Rectangle SourceRect { get; set; }

        /// <summary>
        /// Get or set the destination rectangle when copying part of the page.
        /// </summary>
        public Rectangle DestRect { get; set; }

        /// <summary>
        /// Returns the number of form values.
        /// </summary>
        public int GetFormValueCount() { return FormValues.Count; }

        /// <summary>
        /// Returns the form value at the specified index.
        /// </summary>
        public IGR_Render_Page_Form_Values? GetFormValue(int index)
        {
            if (FormValues.Count > index)
                return FormValues[index];

            return null;
        }

        /// <summary>
        /// Returns the first form value.
        /// </summary>
        public IGR_Render_Page_Form_Values? GetFirstFormValue()
        {
            _formValuesEnumerator = FormValues.GetEnumerator();
            if (_formValuesEnumerator.MoveNext())
                return _formValuesEnumerator.Current;

            return null;
        }

        /// <summary>
        /// Returns the next form value.
        /// </summary>
        public IGR_Render_Page_Form_Values? GetNextFormValue()
        {
            if (_formValuesEnumerator != null && _formValuesEnumerator.MoveNext())
                return _formValuesEnumerator.Current;

            return null;
        }

        /// <summary>
        /// Add a form value to the list of form values.
        /// </summary>
        /// <param name="name">The name of the form element.</param>
        /// <param name="value">The value of the form element.</param>
        /// <param name="selected">Indicates if the value is selected in a list.</param>
        public void AddFormValue(string name, string value, bool selected)
        {
            FormValues.Add(new IGR_Render_Page_Form_Values
            {
                name = name,
                value = value,
                selected = selected ? 1 : 0
            });
        }

        /// <summary>
        /// Add a redaction to the list of redactions.
        /// </summary>
        /// <param name="rect">The bounding rectangle of the redaction.</param>
        /// <param name="color">The color of the redaction.</param>
        public void AddRedaction(Rectangle rect, Color color)
        {
            IGR_Render_Page_Redactions item = new IGR_Render_Page_Redactions();
            item.x = (uint) rect.X;
            item.y = (uint)rect.Y;
            item.width = (uint)rect.Width;
            item.height = (uint)rect.Height;
            item.color = (uint)((color.A << 24) | (color.R << 16) | (color.G << 8) | color.B);
            Redactions.Add(item);
        }
    }
}