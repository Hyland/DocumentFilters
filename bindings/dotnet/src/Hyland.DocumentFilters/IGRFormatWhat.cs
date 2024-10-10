//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// Represents a format supported by the Document Filters API.
    /// </summary>
    public enum IGRFormatWhat
    {
        /// <summary>
        /// Returns the long name of the format.
        /// </summary>
        IGR_FORMAT_LONG_NAME = 0,

        /// <summary>
        /// Returns the short name of the format.
        /// </summary>
        IGR_FORMAT_SHORT_NAME = 1,

        /// <summary>
        /// Returns the configuration name of the format.
        /// </summary>
        IGR_FORMAT_CONFIG_NAME = 2,

        /// <summary>
        /// Returns the class name of the format.
        /// </summary>
        IGR_FORMAT_CLASS_NAME = 3,

        /// <summary>
        /// Returns the legacy name of the format.
        /// </summary>
        IGR_FORMAT_LEGACY = 4,

        /// <summary>
        /// Returns the mime type of the format.
        /// </summary>
        IGR_FORMAT_MIMETYPE = 5,

        /// <summary>
        /// Returns the category of the format.
        /// </summary>
        IGR_FORMAT_FILETYPE_CATEGORY = 6
    }
}
