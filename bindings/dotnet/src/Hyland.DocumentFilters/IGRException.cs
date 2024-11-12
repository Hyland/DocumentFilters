//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections.Generic;
using System.Text;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// Represents an exception thrown by the Document Filters API.
    /// </summary>
    public class IGRException : System.Exception
    {
        private int m_errorCode;

        /// <summary>
        /// Constructs a new IGRException.
        /// </summary>
        public IGRException() { }

        /// <summary>
        /// Constructs a new IGRException with a message.
        /// </summary>
        /// <param name="errorCode">Error code returned by Document Filters</param>
        /// <param name="message">Error message</param>
        public IGRException(int errorCode, string message) 
            : base(message)
        {
            m_errorCode = errorCode;
        }

        /// <summary>
        /// Constructs a new IGRException with a message and an inner exception.
        /// </summary>
        /// <param name="errorCode">Error code returned by Document Filters</param>
        /// <param name="message">Error message</param>
        /// <param name="inner">Inner Exception</param>
        public IGRException(int errorCode, string message, System.Exception inner) 
            : base(message)
        {
            m_errorCode = errorCode;
        }

        /// <summary>
        /// Gets the error code returned by Document Filters.
        /// </summary>
        public int errorCode => m_errorCode;

        /// <summary>
        /// Gets the error code returned by Document Filters.
        /// </summary>
        public int ErrorCode => m_errorCode;

        /// <summary>
        /// Check an Error_Control_Block for an error message and throw an exception if one is found.
        /// </summary>
        /// <param name="ecb">Error Control Block to check.</param>
        /// <param name="errorCode">Error code</param>
        public static void Check(Error_Control_Block ecb, int errorCode = 4)
        {
             if (!String.IsNullOrEmpty(ecb.Msg))
                throw new IGRException(errorCode, ecb.Msg);
        }

        /// <summary>
        /// Check an Error_Control_Block for an error message and throw an exception if one is found.
        /// </summary>
        public static int Check(int resultCode, Error_Control_Block ecb)
        {
            switch (resultCode)
            {
                case ISYS11dfConstants.IGR_OK:
                case ISYS11dfConstants.IGR_NO_MORE:
                    return resultCode;
                default:
                    Check(ecb, resultCode);
                    throw new IGRException(resultCode, $"Unknown Exception: {resultCode}");
            }
        }
    }
}
