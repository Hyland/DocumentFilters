using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

public partial class DocumentFiltersLicense
{
    public static string Get() => Environment.GetEnvironmentVariable("DF_LICENSE_KEY") ?? DocumentFiltersLicense.LICENSE_KEY;
}

