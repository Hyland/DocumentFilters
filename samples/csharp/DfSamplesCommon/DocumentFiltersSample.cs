public partial class DocumentFiltersLicense
{
    public static string Get() => Environment.GetEnvironmentVariable("DF_LICENSE_KEY") ?? DocumentFiltersLicense.LICENSE_KEY;
}

