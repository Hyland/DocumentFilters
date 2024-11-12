//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

namespace Hyland.DocumentFilters
{
#pragma warning disable 1591
    public class IGRTime
    {
        public IGRTime()
        {

        }
        public IGRTime(System.DateTime dt)
        {
            year = dt.Year;
            month = dt.Month;
            day = dt.Day;
            hour = dt.Hour;
            min = dt.Minute;
            sec = dt.Second;
        }
        public int sec { get; set; }
        public int min { get; set; }
        public int hour { get; set; }
        public int day { get; set; }
        public int month { get; set; }
        public int year { get; set; }
        public int weekday { get; set; }
        public int yearday { get; set; }
        public int isdst { get; set; }
    }
}

