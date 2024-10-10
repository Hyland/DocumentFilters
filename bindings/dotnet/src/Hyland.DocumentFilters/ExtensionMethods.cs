using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// Extension methods for the Document Filters API.
    /// </summary>
    public static class ExtensionMethods
    {
        /// <summary>
        /// Converts an IGR_FRect to an IntPtr.
        /// </summary>
        public static IntPtr ToIntPtr(this IGR_FRect? rect)
        {
            if (rect == null) 
                return IntPtr.Zero;
            IntPtr res = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IGR_FRect)));
            Marshal.StructureToPtr(rect, res, false);
            return res;
        }

        /// <summary>
        /// Converts an IGR_FRect to a System.Drawing.RectangleF.
        /// </summary>
        public static System.Drawing.RectangleF ToRectF(this IGR_FRect rect)
            => System.Drawing.RectangleF.FromLTRB(rect.left, rect.top, rect.right, rect.bottom);

        /// <summary>
        /// Converts a System.Drawing.RectangleF to an IGR_FRect.
        /// </summary>
        public static IGR_FRect? ToIgrRectF(this System.Drawing.RectangleF? rect)
        {
            if (rect == null)
                return null;
            return new IGR_FRect { left = rect.Value.Left, top = rect.Value.Top, right = rect.Value.Right, bottom = rect.Value.Bottom };
        }

    }
}
