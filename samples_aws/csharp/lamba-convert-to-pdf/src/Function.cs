using Amazon.Lambda.Core;
using Amazon.Lambda.S3Events;
using Amazon.S3;
using Hyland.DocumentFilters;
using System.Web;

[assembly: LambdaSerializer(typeof(Amazon.Lambda.Serialization.SystemTextJson.DefaultLambdaJsonSerializer))]
namespace DocFilters.Lambda
{
    public class S3ConvertToPDF
    {
        private IAmazonS3 _S3Client;
        private Hyland.DocumentFilters.Api _DocFiltersApi;

        public S3ConvertToPDF() : this(new AmazonS3Client())
        {
        }

        public S3ConvertToPDF(IAmazonS3 s3Client)
        {
            _S3Client = s3Client;
            _DocFiltersApi = new Hyland.DocumentFilters.Api();

            /* You can insert DocFilters license here. Check READ.me to learn more. */
            string docfilters_license_key = "";
            _DocFiltersApi.Initialize(docfilters_license_key, ".");
        }

        public async Task<string?> FunctionHandler(S3Event evnt, ILambdaContext context)
        {
            var eventRecords = evnt?.Records ?? new List<S3Event.S3EventNotificationRecord>();
            foreach (var record in eventRecords)
            {
                var s3Event = record.S3;
                if (s3Event == null)
                    continue;

                try
                {
                    Stream? strm = await CreateSeekableStream(await _S3Client.GetObjectStreamAsync(s3Event.Bucket.Name, s3Event.Object.Key, null));
                    if (strm == null)
                        throw new ArgumentNullException(nameof(strm), $"Failed to download stream of {s3Event.Bucket.Name}/{s3Event.Object.Key}");

                    var destinationBucket = Environment.GetEnvironmentVariable("S3_DEST_BUCKET") ?? $"{s3Event.Bucket.Name}-pdf";
                    var destinationName = Path.ChangeExtension(s3Event.Object.Key, ".pdf");

                    var outputStream = new MemoryStream();

                    // Open the document and convert it to a PDF stream
                    {
                        using var doc = _DocFiltersApi.OpenExtractor(strm, OpenMode.Paginated, options: "PDF_PRESERVE_ORIGINAL=on");
                        using var canvas = _DocFiltersApi.MakeOutputCanvas(outputStream, CanvasType.PDF);

                        canvas.RenderPages(doc);
                    }

                    var putResponse = await _S3Client.PutObjectAsync(new Amazon.S3.Model.PutObjectRequest
                    {
                        BucketName = destinationBucket,
                        Key = destinationName,
                        InputStream = outputStream,
                        AutoCloseStream = true,
                        AutoResetStreamPosition = true,
                        ContentType = "application/pdf",
                    });

                    if (putResponse.HttpStatusCode == System.Net.HttpStatusCode.OK || putResponse.HttpStatusCode == System.Net.HttpStatusCode.Created)
                        context.Logger.LogError($"PDF written as {destinationName} to S3 bucket {destinationBucket}");
                    else
                        context.Logger.LogError($"Failed to write PDF to S3 bucket with error {putResponse.HttpStatusCode}");

                    return $"https://{destinationBucket}.s3.amazonaws.com/{HttpUtility.UrlEncode(destinationName)}";
                }
                catch (Exception e)
                {
                    context.Logger.LogError($"Error getting object {s3Event.Object.Key} from bucket {s3Event.Bucket.Name}. Make sure they exist and your bucket is in the same region as this function.");
                    context.Logger.LogError(e.Message);
                    context.Logger.LogError(e.StackTrace);
                    throw;
                }
            }
            return null;
        }

        private static async Task<Stream?> CreateSeekableStream(Stream? strm)
        {
            if (strm == null)
                return null;

            Stream result = strm.Length < 1048576
                ? new MemoryStream()
                : new FileStream(Path.GetTempFileName(), FileMode.Create, FileAccess.ReadWrite, FileShare.None, 4096, FileOptions.DeleteOnClose | FileOptions.RandomAccess);

            await strm.CopyToAsync(result);
            result.Position = 0;
            return result;
        }
    }
}