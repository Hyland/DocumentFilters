using System;
using System.Collections.Generic;
using System.Text;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// Represents the settings for comparing documents.
    /// </summary>
    public class CompareDocumentSettings
    {
        /// <summary>
        /// Gets or sets the first page to compare.
        /// </summary>
        public int FirstPage { get; set; }

        /// <summary>
        /// Gets or sets the number of pages to compare.
        /// </summary>
        public int PageCount { get; set; }

        /// <summary>
        /// Gets or sets the margins to use when comparing documents.
        /// </summary>
        public System.Drawing.RectangleF? Margins { get; set; }
    }

    /// <summary>
    /// Represents the source document to compare.
    /// </summary>
    public class CompareDocumentSource : CompareDocumentSettings
    {
        /// <summary>
        /// Gets or sets the extractor to use for the source document.
        /// </summary>
        public Extractor Extractor { get; set; }

        /// <summary>
        /// Internal method to create the IGR_Text_Compare_Document_Source object.
        /// </summary>
        internal IGR_Text_Compare_Document_Source Make()
        {
            return new IGR_Text_Compare_Document_Source
            {
                struct_size = (uint)Marshaler.SizeOf<IGR_Text_Compare_Document_Source>(),
                doc_handle = Extractor.Handle,
                doc_first_page = (uint)FirstPage,
                doc_page_count = (uint)PageCount,
                doc_margins = Margins != null ? Margins.ToIgrRectF().Value : new IGR_FRect()
            };
        }
    }
}
