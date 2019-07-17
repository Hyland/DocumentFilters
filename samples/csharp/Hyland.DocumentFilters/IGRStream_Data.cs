//===========================================================================
// (c) 2018 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;

namespace Hyland.DocumentFilters
{
    public delegate void IGRStreamAction(byte[] data, int len);

    public class IGRStream_Data 
    {
        private IGRStreamAction callback;

        public IGRStream_Data(IGRStreamAction cb)
        {
            this.callback = cb;
        }
        public void write(byte[] byteArray, int len)
        {
            callback(byteArray, len);
        }
    }
}
