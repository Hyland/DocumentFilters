using System;
using System.Collections.Generic;
using System.Text;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// Represents the comparison type used when comparing documents.
    /// </summary>
    public enum CompareType
    {
        /// <summary>
        /// Indicates that the comparison should be done at the word level.
        /// </summary>
        Word = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_COMPARE_WORDS
    }

    /// <summary>
    /// Represents the flags used when comparing documents.
    /// </summary>
    [Flags]
    public enum CompareFlags
    {
        /// <summary>
        /// Indicates that no flags are set.
        /// </summary>
        None = 0,
        
        /// <summary>
        /// Indicates that content that matches both documents should be emitted when enumerating differences.
        /// </summary>
        Equals = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_EQUALS,
        
        /// <summary>
        /// Reserved for future use.
        /// </summary>
        Moves = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_MOVES,
        
        /// <summary>
        /// Indicates that formatting should be compared.
        /// </summary>
        Formatting = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_FORMATTING,
        
        /// <summary>
        /// Indicates that comments should be compared.
        /// </summary>
        NoComments = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_NO_COMMENTS,

        /// <summary>
        /// Indicates if the comparison should be case sensitive.
        /// </summary>
        NoCase = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE,

        /// <summary>
        /// Indicates if the comparison should ignore whitespace.
        /// </summary>
        NoWhitespace = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_NO_WHITESPACE,

        /// <summary>
        /// Indicates if the comparison should ignore punctuation.
        /// </summary>
        NoPunctuation = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_NO_PUNCTUATION,

        /// <summary>
        /// Indicates if the comparison should ignore tables.
        /// </summary>
        NoTables = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_NO_TABLES,

        /// <summary>
        /// Indicates if the comparison should ignore headers.
        /// </summary>
        NoHeaders = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS,

        /// <summary>
        /// Indicates if the comparison should ignore footers.
        /// </summary>
        NoFooters = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS,

        /// <summary>
        /// Indicates if the comparison should ignore footnotes.
        /// </summary>
        NoFootnotes = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTNOTES,

        /// <summary>
        /// Indicates if the comparison should ignore text boxes.
        /// </summary>
        NoTextBoxes = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_NO_TEXTBOXES,

        /// <summary>
        /// Indicates if the comparison should ignore fields.
        /// </summary>
        NoFields = ISYS11dfConstants.IGR_COMPARE_DOCUMENTS_FLAGS_NO_FIELDS,
    }

    /// <summary>
    /// Represents the settings for comparing documents.
    /// </summary>
    public class CompareSettings
    {
        /// <summary>
        /// Get or sets the comparison type.
        /// </summary>
        public CompareType CompareType { get; set; } = CompareType.Word;

        /// <summary>
        /// Get or set the flags to use when comparing documents.
        /// </summary>
        public CompareFlags Flags { get; set; } = CompareFlags.None;

        /// <summary>
        /// Internal method to create the IGR_Text_Compare_Settings object.
        /// </summary>
        internal IGR_Text_Compare_Settings Make()
        {
            IGR_Text_Compare_Settings res = new IGR_Text_Compare_Settings();
            res.struct_size = (uint)Marshaler.SizeOf<IGR_Text_Compare_Settings>();
            res.flags = (uint)Flags;
            res.compare_type = (uint)CompareType;
            return res;
        }
    }
}
