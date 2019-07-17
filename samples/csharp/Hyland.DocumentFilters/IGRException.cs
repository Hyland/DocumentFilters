//===========================================================================
// (c) 2018 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections.Generic;
using System.Text;

namespace Hyland.DocumentFilters
{
    public class IGRException : System.Exception
    {
        private int m_errorCode;

        public IGRException() { }
        public IGRException(int errorCode, string message) 
            : base(message)
        {
            m_errorCode = errorCode;
        }
        public IGRException(int errorCode, string message, System.Exception inner) 
            : base(message)
        {
            m_errorCode = errorCode;
        }

        public int errorCode
        {
            get
            {
                return m_errorCode;
            }
        }

        public static void Check(Error_Control_Block ecb, int errorCode = 4)
        {
             if (!String.IsNullOrEmpty(ecb.Msg))
                throw new IGRException(errorCode, ecb.Msg);
        }
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
