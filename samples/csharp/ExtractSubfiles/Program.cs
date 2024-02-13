/*
   (c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
using Hyland.DocumentFilters;
using McMaster.Extensions.CommandLineUtils;
using Microsoft.Extensions.FileSystemGlobbing;

class Program
{
    [Argument(0, Description = "Container file to process")]
    public string? FileName { get; set; }

    [Argument(1, Description = "Patterns of files to extract")]
    public List<string> IncludeFiles { get; set; } = new List<string>();

    [Option("-r|--recurse", Description = "Recursively process extracted subfiles")]
    public bool Recurse { get; set; }

    [Option("--max-depth", Description = "The maximum depth subfiles should be recursed into")]
    public int MaxDepth { get; set; } = Int32.MaxValue;

    [Option("-l|--list", Description = "List subfiles")]
    public bool List { get; set; }

    [Option("-d|--dir", Description = "Extract files into directory")]
    public string Dir { get; set; } = ".";

    [Option("--flatten-dirs", Description = "Flatten directory names in output")]
    public bool Flatten { get; set; }

    [Option("--no-interactive", Description = "Disable password prompt")]
    public bool NoInteractive { get; set; } = false;

    private readonly Hyland.DocumentFilters.Api _api = new();
    private readonly Matcher _matcher = new();

    public int OnExecute()
    {
        _api.Initialize(DocumentFiltersLicense.Get(), ".");

        if (string.IsNullOrEmpty(FileName))
            throw new ArgumentNullException(nameof(FileName));

        _matcher.AddIncludePatterns(IncludeFiles);
        if (IncludeFiles.Count == 0)
            _ = _matcher.AddInclude("**/*");

        Dir = Path.GetFullPath(Dir);

        using Extractor file = _api.GetExtractor(FileName);

        ProcessFile(file, FileName);

        return 0;
    }

    private string ReadPassword()
    {
        string result = "";

        ConsoleKeyInfo nextKey = Console.ReadKey(true);

        while (nextKey.Key != ConsoleKey.Enter)
        {
            if (nextKey.Key == ConsoleKey.Backspace)
            {
                if (result.Length > 0)
                    _ = result.Remove(result.Length - 1);
            }
            else
            {
                result += nextKey.KeyChar;
            }
            nextKey = Console.ReadKey(true);
        }
        return result;
    }

    private void ProcessFile(Extractor extractor, string fileName, int depth = 0)
    {
        Func<OpenCallback, int> openCallback = (OpenCallback req) =>
        {
            switch (req.Action)
            {
                case ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_PASSWORD:
                    if (!string.IsNullOrWhiteSpace(req.Password.id))
                        Console.Out.WriteLine($"Password required for \"{req.Password.id}\"");
                    if (NoInteractive)
                        return ISYS11dfConstants.IGR_E_PASSWORD;
                    Console.Out.Write("Password: ");
                    req.Password.password = ReadPassword();
                    Console.Out.WriteLine();
                    return ISYS11dfConstants.IGR_OK;
                default:
                    return ISYS11dfConstants.IGR_OK;
            }
        };

        Console.Error.WriteLine($"Container: {fileName}");
        try
        {
            extractor.Open(OpenMode.Text, OpenType.BodyOnly, "", openCallback);

            foreach (SubFile? subfile in extractor.SubFiles)
            {
                using (subfile)
                {
                    if (List)
                    {
                        string dt = subfile.FileDate.Year > 1970
                            ? $"{subfile.FileDate:dd MMM yyyy hh:mm:ss tt}"
                            : "<...no date...>";

                        Console.Out.WriteLine($"{dt,-24} {subfile.FileSize,14:N0} {subfile.Name}");
                    }
                    else if (_matcher.Match(subfile.Name).HasMatches)
                    {
                        // Extract
                        Console.WriteLine($"  extracting: {subfile.Name}");

                        // Normalize slashes to the host's directory seperator
                        string destFileName = subfile.Name
                            .Replace('/', Path.DirectorySeparatorChar)
                            .Replace('\\', Path.DirectorySeparatorChar);

                        // If flattening, changes the slashed to underscore
                        if (Flatten)
                            destFileName = destFileName.Replace(Path.DirectorySeparatorChar, '_');

                        // Get the absolute path to the output
                        destFileName = Path.GetFullPath(Path.Combine(Dir, destFileName));

                        // Make sure it's not a malformed file trying to write outside of the output dir.
                        if (!destFileName.StartsWith(Dir))
                            destFileName = Path.Combine(Dir, Path.GetFileName(destFileName));

                        string? dir = Path.GetDirectoryName(destFileName);
                        if (dir != null)
                            _ = Directory.CreateDirectory(dir);
                        subfile.CopyTo(destFileName);
                    }

                    if (Recurse && depth <= MaxDepth)
                        ProcessFile(subfile, fileName + "|" + subfile.Name, depth + 1);
                }
            }
        }
        catch (Exception e)
        {
            Console.Error.WriteLine($"Error: {e.Message}");
        }
    }

    static int Main(string[] args) => CommandLineApplication.Execute<Program>(args);
}

