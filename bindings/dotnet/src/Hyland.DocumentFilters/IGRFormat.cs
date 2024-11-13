//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections.Generic;
using System.Text;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// Represents a format supported by the Document Filters API.
    /// </summary>
    public class Format
    {
        /// <summary>
        /// Represents the class of a format.
        /// </summary>
        public enum FormatClass
        {
            /// <summary>
            /// Indicates that the format is an office productivity format.
            /// </summary>
            OfficeProductivity,

            /// <summary>
            /// Indicates that the format is a text format.
            /// </summary>
            Text,

            /// <summary>
            /// Indicates that the format is a media format.
            /// </summary>
            Media,

            /// <summary>
            /// Indicates that the format is a container format.
            /// </summary>
            Container,

            /// <summary>
            /// Indicates that the format is an other format.
            /// </summary>
            Other
        }

        /// <summary>
        /// Gets the display name of the format.
        /// </summary>
        public string DisplayName { get; private set; }

        /// <summary>
        /// Gets the short name of the format.
        /// </summary>
        public string ShortName { get; private set; }

        /// <summary>
        /// Gets the configuration name of the format.
        /// </summary>
        public string ConfigName { get; private set; }

        /// <summary>
        /// Gets the MIME type of the format.
        /// </summary>
        public string MimeType { get; private set; }

        /// <summary>
        /// Gets the class of the format.
        /// </summary>
        public FormatClass Class { get; private set; }

        /// <summary>
        /// Gets the ID of the format.
        /// </summary>
        public int Id { get; private set; }

        /// <summary>
        /// Gets the file type category of the format.
        /// </summary>
        public int FileTypeCategory { get; private set; }

        /// <summary>
        /// Constructs a new Format.
        /// </summary>
        internal Format(DocumentFilters api, int type, string name)
        {
            Id = type;
            DisplayName = name ?? fetch(type, IGRFormatWhat.IGR_FORMAT_LONG_NAME);
            ShortName = fetch(type, IGRFormatWhat.IGR_FORMAT_SHORT_NAME);
            ConfigName = fetch(type, IGRFormatWhat.IGR_FORMAT_CONFIG_NAME);
            MimeType = fetch(type, IGRFormatWhat.IGR_FORMAT_MIMETYPE);
            FileTypeCategory = int.TryParse(fetch(type, IGRFormatWhat.IGR_FORMAT_FILETYPE_CATEGORY), out int category) ? category : 0;
            switch (fetch(type, IGRFormatWhat.IGR_FORMAT_CLASS_NAME))
            {
                case "P":
                    Class = FormatClass.OfficeProductivity;
                    break;
                case "T":
                    Class = FormatClass.Text;
                    break;
                case "M":
                    Class = FormatClass.Media;
                    break;
                case "C":
                    Class = FormatClass.Container;
                    break;
                case "O":
                    Class = FormatClass.Other;
                    break;
            }
        }

        internal static IEnumerable<Format> Fetch(DocumentFilters api)
        {
            string name;
            for (int i = 0; i < 1024; ++i)
            {
                if (fetch(i, IGRFormatWhat.IGR_FORMAT_LONG_NAME, out name))
                {
                    yield return new Format(api, i, name);
                }
            }            
        }
        internal static string fetch(int type, IGRFormatWhat what)
        {
            string res;
            fetch(type, what, out res);
            return res;
        }
        internal static bool fetch(int type, IGRFormatWhat what, out string result)
        {
            Error_Control_Block ecb = new Error_Control_Block();
            StringBuilder buffer = new StringBuilder(255);
            if (ISYS11df.IGR_Get_Format_Attribute(type, (int) what, buffer, ref ecb) == 0)
            {
                result = buffer.ToString();
                return true;
            }
            else
            {
                result = "";
                return false;
            }
        }
    }

}