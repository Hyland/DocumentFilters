//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections.Generic;
using System.Text;

namespace Hyland.DocumentFilters
{

    /// <summary>
    /// Represents an option that can be set on the Document Filters API.
    /// </summary>
    public class Option
    {
        /// <summary>
        /// Gets the display name of the option.
        /// </summary>
        public string DisplayName { get; internal set; }

        /// <summary>
        /// Gets the description of the option.
        /// </summary>
        public string Description { get; internal set; }

        /// <summary>
        /// Gets the default value of the option.
        /// </summary>
        public string DefaultValue { get; internal set; }

        /// <summary>
        /// Gets the possible values of the option.
        /// </summary>
        public IEnumerable<string> PossibleValues { get; internal set; }

        /// <summary>
        /// Gets the type of the option.
        /// </summary>
        public string Type { get; internal set; }

        /// <summary>
        /// Gets the flags of the option.
        /// </summary>
        public int Flags { get; internal set; }

        internal static IEnumerable<Option> Fetch(DocumentFilters api)
        {
            string name, description, def, type, vals, flags;

            for (int i = 0; i < 1024*64 && fetch(i, 0, out name); ++i)
            {
                fetch(i, 1, out description);
                fetch(i, 2, out def);
                fetch(i, 3, out flags);
                fetch(i, 4, out type);
                fetch(i, 5, out vals);

                yield return new Option
                {
                    DisplayName = name,
                    Description = description,
                    DefaultValue = def,
                    Type = type,
                    PossibleValues = vals.Split(new char[] { ';', '|', ',' }, StringSplitOptions.RemoveEmptyEntries),
                    Flags = int.TryParse(flags, out var f)  ? f : 0
                };
            }            
        }

        internal static bool fetch(int index, int what, out string result)
        {
            Error_Control_Block ecb = new Error_Control_Block();
            StringBuilder buffer = new StringBuilder(4096);
            if (ISYS11df.IGR_Get_Option_Attribute(index, what, buffer, new System.IntPtr(buffer.Capacity), ref ecb) == 0)
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