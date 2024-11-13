//===========================================================================
// (c) 2020 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System;

namespace Hyland.DocumentFilters
{

    /// <summary>
    /// The Hyperlink interface represents a single hyperlink on a given page.
    /// </summary>
    public class Hyperlink : DocumentFiltersBase
    {
        private IGR_Hyperlink _hyperlink;

        internal Hyperlink(IGR_Hyperlink hyperlink)
        {
            _hyperlink = hyperlink;  
        }

        /// <summary>
        /// Represents the type of action that the hyperlink performs.
        /// </summary>
        [System.Flags]
        public enum ActionType
        {
            /// <summary>
            /// Indicates that the hyperlink goes to a location in the document.
            /// </summary>
            GoTo = ISYS11dfConstants.IGR_HYPERLINK_ACTION_GOTO,

            /// <summary>
            /// Indicates that the hyperlink goes to a URI.
            /// </summary>
            URI = ISYS11dfConstants.IGR_HYPERLINK_ACTION_URI
        }

        /// <summary>
        /// Represents the flags that can be set on a hyperlink.
        /// </summary>
        [System.Flags]
        public enum HyperlinkFlags
        {
            /// <summary>
            /// Indicates that the hyperlink changes the left position.
            /// </summary>
            ChangesLeft = ISYS11dfConstants.IGR_HYPERLINK_FLAGS_CHANGE_LEFT,

            /// <summary>
            /// Indicates that the hyperlink changes the top position.
            /// </summary>
            ChangesTop = ISYS11dfConstants.IGR_HYPERLINK_FLAGS_CHANGE_TOP,

            /// <summary>
            /// Indicates that the hyperlink changes the zoom.
            /// </summary>
            ChangesZoom = ISYS11dfConstants.IGR_HYPERLINK_FLAGS_CHANGE_ZOOM,
        }

        /// <summary>
        /// Represents the type of fit that the hyperlink uses.
        /// </summary>
        public enum FitType
        {
            /// <summary>
            /// Indicates that the hyperlink fits the XYZ coordinates.
            /// </summary>
            XYZ = ISYS11dfConstants.IGR_HYPERLINK_FIT_XYZ,

            /// <summary>
            /// Indicates that the hyperlink fits the page.
            /// </summary>
            Fit = ISYS11dfConstants.IGR_HYPERLINK_FIT_FIT,

            /// <summary>
            /// Indicates that the hyperlink fits the width of the page.
            /// </summary>
            FitH = ISYS11dfConstants.IGR_HYPERLINK_FIT_FITH,

            /// <summary>
            /// Indicates that the hyperlink fits the height of the page.
            /// </summary>
            FitV = ISYS11dfConstants.IGR_HYPERLINK_FIT_FITV,

            /// <summary>
            /// Indicates that the hyperlink fits the rectangle.
            /// </summary>
            FitR = ISYS11dfConstants.IGR_HYPERLINK_FIT_FITR,

            /// <summary>
            /// Indicates that the hyperlink fits the bounding box.
            /// </summary>
            FitB = ISYS11dfConstants.IGR_HYPERLINK_FIT_FITB,

            /// <summary>
            /// Indicates that the hyperlink fits the bounding box of the page.
            /// </summary>
            FitBH = ISYS11dfConstants.IGR_HYPERLINK_FIT_FITBH,

            /// <summary>
            /// Indicates that the hyperlink fits the bounding box of the page.
            /// </summary>
            FitBV = ISYS11dfConstants.IGR_HYPERLINK_FIT_FITBV,
        }

        /// <summary>
        /// Gets the type of action that the hyperlink performs.
        /// </summary>
        public ActionType Type => (ActionType)_hyperlink.type;

        /// <summary>
        /// Gets the X coordinate of the hyperlink.
        /// </summary>
        public int X => _hyperlink.x;

        /// <summary>
        /// Gets the Y coordinate of the hyperlink.
        /// </summary>
        public int Y => _hyperlink.y;

        /// <summary>
        /// Gets the width of the hyperlink.
        /// </summary>
        public int Width => _hyperlink.width;

        /// <summary>
        /// Gets the height of the hyperlink.
        /// </summary>
        public int Height => _hyperlink.height;

        /// <summary>
        /// Gets the page number of the hyperlink.
        /// </summary>
        public int PageNumber => _hyperlink.page_number;

        /// <summary>
        /// Gets the destination fit type of the hyperlink.
        /// </summary>
        public FitType DestFit => (FitType)_hyperlink.dest_fit;

        /// <summary>
        /// Gets the destination left coordinate of the hyperlink.
        /// </summary>
        public int DestLeft => _hyperlink.dest_left;

        /// <summary>
        /// Gets the destination top coordinate of the hyperlink.
        /// </summary>
        public int DestTop => _hyperlink.dest_top;

        /// <summary>
        /// Gets the destination right coordinate of the hyperlink.
        /// </summary>
        public int DestRight => _hyperlink.dest_right;

        /// <summary>
        /// Gets the destination bottom coordinate of the hyperlink.
        /// </summary>
        public int DestBottom => _hyperlink.dest_bottom;

        /// <summary>
        /// Gets the flags set on the hyperlink.
        /// </summary>
        public HyperlinkFlags Flags => (HyperlinkFlags)_hyperlink.flags;

        /// <summary>
        /// Gets the reference of the hyperlink.
        /// </summary>
        public string Ref => GetStr(ISYS11dfConstants.IGR_HYPERLINK_GET_VALUE_REF);

        /// <summary>
        /// Gets the URI of the hyperlink.
        /// </summary>
        public string Uri => GetStr(ISYS11dfConstants.IGR_HYPERLINK_GET_VALUE_URI);

        private string GetStr(int type, int maxLength = 4096)
        {
            Error_Control_Block ecb = new Error_Control_Block();
            StringBuilder res = new StringBuilder(maxLength);

            Check(ISYS11df.IGR_Get_Page_Hyperlink_Str(ref _hyperlink, type, res.Capacity, res, ref ecb), ecb);
            return res.ToString();
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        /// <summary>
        /// Disposes of the hyperlink object.
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
        /// Disposes of the hyperlink object.
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

