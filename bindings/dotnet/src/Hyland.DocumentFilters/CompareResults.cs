using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// Represents the type of difference between two documents.
    /// </summary>
    public enum DifferenceType
    {
        /// <summary>
        /// Indicates that the difference hit are equal.
        /// </summary>
        Equal = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL,

        /// <summary>
        /// Indicates that the difference hit are insertions.
        /// </summary>
        Insert = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT,

        /// <summary>
        /// Indicates that the difference hit are deletions.
        /// </summary>
        Delete = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE,

        /// <summary>
        /// Indicates that the difference hit are due to formatting difference.
        /// </summary>
        Formatting = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_DIFFERENCE_FORMATTING,

        /// <summary>
        /// Indicates that a next batch is about to be requested.
        /// </summary>
        NextBatch = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH,
    }

    /// <summary>
    /// Represents the source of the difference.
    /// </summary>
    public enum DifferenceSource
    {
        /// <summary>
        /// Indicates that the difference is from the original document.
        /// </summary>
        Original = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_ORIGINAL,

        /// <summary>
        /// Indicates that the difference is from the revised document.
        /// </summary>
        Revised = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_REVISED,

        /// <summary>
        /// Indicates that the difference is from both the original and revised documents.
        /// </summary>
        Both = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_BOTH,
    }

    /// <summary>
    /// Represents the details of a difference between two documents.
    /// </summary>
    public class CompareResultDifferenceDetail
    {
        /// <summary>
        /// Indicates the page index where the difference is found.
        /// </summary>
        public int PageIndex { get; internal set; }

        /// <summary>
        /// Indicates the bounds of the difference.
        /// </summary>
        public System.Drawing.RectangleF Bounds { get; internal set; }

        /// <summary>
        /// Indicates the text of the difference.
        /// </summary>
        public string Text { get; internal set; }


        /// <summary>
        /// Internal method to create the CompareResultDifferenceDetail object.
        /// </summary>
        internal static CompareResultDifferenceDetail Make(IGR_Compare_Documents_Difference_Item info)
        {
            return new CompareResultDifferenceDetail
            {
                Text = info.text,
                PageIndex = (int)info.page_index,
                Bounds = System.Drawing.RectangleF.FromLTRB(info.bounds.left, info.bounds.top, info.bounds.right, info.bounds.bottom)
            };
        }

        /// <summary>
        /// ToString override.
        /// </summary>
        public override string ToString() => $"{Text} [{Bounds}]";
    }

    /// <summary>
    /// Represents one difference between two documents.
    /// </summary>
    public class CompareResultDifference
    {
        /// <summary>
        /// Indicates the type of difference.
        /// </summary>
        public DifferenceType Type { get; internal set; }

        /// <summary>
        /// Indicates the source of the difference.
        /// </summary>
        public DifferenceSource Source { get; internal set; }

        /// <summary>
        /// Indicates the original page index where the difference is found.
        /// </summary>
        public int OriginalPageIndex { get; internal set; }

        /// <summary>
        /// Indicates the revised page index where the difference is found.
        /// </summary>
        public int RevisedPageIndex { get; internal set; }

        /// <summary>
        /// Indicates the details of the difference.
        /// </summary>
        public IEnumerable<CompareResultDifferenceDetail> Details { get; internal set; }

        /// <summary>
        /// Gets the text of the difference.
        /// </summary>
        /// <returns></returns>
        public string Text()
        {
            if (Details == null)
                return "";

            StringBuilder stringBuilder = new StringBuilder();
            System.Drawing.RectangleF? last = null;

            foreach (CompareResultDifferenceDetail detail in Details)
            {
                if (last != null)
                {
                    if (detail.Bounds.Top > last.Value.Top)
                        stringBuilder.Append("\n");
                    else if (detail.Bounds.Left - 2 > last.Value.Right)
                        stringBuilder.Append(" ");
                }

                stringBuilder.Append(detail.Text);
                last = detail.Bounds;
            }

            return stringBuilder.ToString();
        }

        /// <summary>
        /// Internal method to create the CompareResultDifference object.
        /// </summary>
        internal static CompareResultDifference Make(IGR_Compare_Documents_Difference info)
        {
            List<CompareResultDifferenceDetail> details = new List<CompareResultDifferenceDetail>();

            IntPtr itemPtr = info.items;
            for (uint i = 0; i < info.item_count; ++i)
            {
                var detail = Marshaler.PtrToStructure<IGR_Compare_Documents_Difference_Item>(itemPtr);
                if (detail.struct_size == 0)
                    break;
                details.Add(CompareResultDifferenceDetail.Make(detail));
                itemPtr = (IntPtr)(itemPtr.ToInt64() + detail.struct_size);
            }

            return new CompareResultDifference
            {
                Type = (DifferenceType)info.type,
                Source = (DifferenceSource)info.doc_source,
                OriginalPageIndex = (int)info.original_page_index,
                RevisedPageIndex = (int)info.revised_page_index,
                Details = details
            };
        }

        /// <summary>
        /// ToString override.
        /// </summary>
        public override string ToString() => $"[{Type}] {Source} {(Details != null ? string.Join(", ", Details.Select(x => x.ToString()).ToArray()) : "")}";
    }

    /// <summary>
    /// Represents the results of comparing two documents.
    /// </summary>
    public class CompareResults : IDisposable, IEnumerator<CompareResultDifference>
    {
        private IntPtr _handle;
        private bool _disposedValue;
        private CompareResultDifference _current;

        /// <summary>
        /// Gets the current difference.
        /// </summary>
        public CompareResultDifference Current => _current;

        /// <summary>
        /// Gets the current difference.
        /// </summary>
        object IEnumerator.Current => _current;

        /// <summary>
        /// Internal method to create the CompareResults object.
        /// </summary>
        internal static CompareResults Make(IntPtr enumerator) => new CompareResults(enumerator);

        /// <summary>
        /// Internal method to create the CompareResults object.
        /// </summary>
        internal CompareResults(IntPtr handle)
        {
            _handle = handle;
        }

        /// <summary>
        /// Close the comparison.
        /// </summary>
        public void Close()
        {
            if (_handle != IntPtr.Zero)
            {
                Error_Control_Block ecb = new Error_Control_Block();
                ISYS11df.IGR_Text_Compare_Close(_handle, ref ecb);
                _handle = IntPtr.Zero;
            }
        }

        /// <summary>
        /// Returns the next difference.
        /// </summary>
        public CompareResultDifference GetNext()
        {
            IGR_Compare_Documents_Difference n = new IGR_Compare_Documents_Difference();
            n.struct_size = (uint)Marshaler.SizeOf<IGR_Compare_Documents_Difference>();
            Error_Control_Block ecb = new Error_Control_Block();

            if (ISYS11df.IGR_Text_Compare_Next(_handle, ref n, ref ecb) == 0)
            {
                _current = CompareResultDifference.Make(n);
                ISYS11df.IGR_Text_Compare_Difference_Dispose(ref n, ref ecb);
            }
            else
                _current = null;

            return _current;
        }

        /// <summary>
        /// Dispose the object.
        /// </summary>
        protected virtual void Dispose(bool disposing)
        {
            if (!_disposedValue)
            {
                if (disposing)
                    Close();

                _disposedValue = true;
            }
        }

        /// <summary>
        /// Dispose the object.
        /// </summary>
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Move to the next difference.
        /// </summary>
        public bool MoveNext() => GetNext() != null;

        /// <summary>
        /// Reset the enumerator.
        /// </summary>
        public void Reset()
        {
        }

        /// <summary>
        /// Returns the enumerator.
        /// </summary>
        /// <returns></returns>
        public IEnumerator<CompareResultDifference> GetEnumerator() => this;

        /// <summary>
        /// Convert the differences to a list.
        /// </summary>
        /// <returns></returns>
        public IList<CompareResultDifference> ToList()
        {
            List<CompareResultDifference> result = new List<CompareResultDifference>();
            while (MoveNext())
                result.Add(_current);
            return result;
        }
    }
}
