//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections.Generic;
using System.Text;

namespace Hyland.DocumentFilters
{

    /// <summary>
    /// Base class for all Document Filters classes.
    /// </summary>
    public class DocumentFiltersBase
    {
        /// <summary>
        /// Error control block for the Document Filters API.
        /// </summary>
        protected Error_Control_Block ecb = new Error_Control_Block();

        /// <summary>
        /// Internal method to check the exit code and throw an exception if necessary.
        /// </summary>
        internal static int Check(int exitCode, Error_Control_Block ecb)
        {
            IGRException.Check(exitCode, ecb);
            return exitCode;
        }

        /// <summary>
        /// Internal method to check the exit code and throw an exception if necessary.
        /// </summary>
        protected int Check(int exitCode)
        {
            return Check(exitCode, ecb);
        }

        /// <summary>
        /// Internal method to check the exit code and throw an exception if necessary.
        /// </summary>
        protected static void VerifyArgumentNotNull<T>(T obj, string argumentName)
            where T : class
        {
            if (obj == null)
            {
                throw new ArgumentNullException(argumentName);
            }
        }

        /// <summary>
        /// Internal method to check the exit code and throw an exception if necessary.
        /// </summary>
        protected static void VerifyArgumentNotEmpty(string value, string argumentName)
        {
            if (String.IsNullOrEmpty(value))
            {
                throw new ArgumentException($"{argumentName} cannot be empty");
            }
        }

        /// <summary>
        /// Internal method to check the exit code and throw an exception if necessary.
        /// </summary>
        protected static void VerifyArgumentInRange(long value, long min, long max, string argumentName)
        {
            if (value < min || value > max)
                throw new ArgumentOutOfRangeException(argumentName);
        }
    }
}
