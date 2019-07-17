//===========================================================================
// (c) 2018 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections.Generic;
using System.Text;

namespace Hyland.DocumentFilters
{
    public class DocumentFiltersBase
    {
        protected Error_Control_Block ecb = new Error_Control_Block();

        internal static int Check(int exitCode, Error_Control_Block ecb)
        {
            IGRException.Check(exitCode, ecb);
            return exitCode;
        }
        protected int Check(int exitCode)
        {
            return Check(exitCode, ecb);
        }
        protected static void VerifyArgumentNotNull<T>(T obj, string argumentName)
            where T : class
        {
            if (obj == null)
            {
                throw new ArgumentNullException(argumentName);
            }
        }
        protected static void VerifyArgumentNotEmpty(string value, string argumentName)
        {
            if (String.IsNullOrEmpty(value))
            {
                throw new ArgumentException($"{argumentName} cannot be empty");
            }
        }

        protected static void VerifyArgumentInRange(long value, long min, long max, string argumentName)
        {
            if (value < min || value > max)
                throw new ArgumentOutOfRangeException(argumentName);
        }
    }
}
