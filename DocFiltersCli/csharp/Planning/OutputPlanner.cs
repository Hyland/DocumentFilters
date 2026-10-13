/*
   (c) 2026 Hyland Software, Inc. and its affiliates. All rights reserved.

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

using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Discovery;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Planning
{
	/// <summary>
	/// Decides where every output goes, before anything is written.
	/// </summary>
	/// <remarks>
	/// The one rule underneath all of this: <b>the meaning of <c>-o</c> never depends
	/// on <c>--to</c></b>. Four cardinalities, chosen by the arguments alone:
	/// <list type="bullet">
	/// <item><see cref="OutputCardinality.PerInput"/> — <c>--out-dir</c>, each input writes its own output</item>
	/// <item><see cref="OutputCardinality.Combined"/> — <c>--combine --out-file book.pdf</c></item>
	/// <item><see cref="OutputCardinality.SingleTarget"/> — <c>--out-file</c> with exactly one input</item>
	/// <item><see cref="OutputCardinality.Stream"/> — <c>--stdout</c>, or <c>--out-file -</c></item>
	/// </list>
	/// These name target ownership only. How many <em>files</em> one input produces is
	/// the separate question <see cref="FormatSpec.IsSingleFile"/> answers: a .docx to
	/// PNG writes one file per page and is still <see cref="OutputCardinality.PerInput"/>.
	///
	/// <c>--out-file</c> with several inputs and no <c>--combine</c> is a hard error,
	/// never a guess.
	/// </remarks>
	public static class OutputPlanner
	{
		/// <summary>
		/// Builds the plan. Pure: no filesystem access, so it is exhaustively testable
		/// with a table of cases.
		/// </summary>
		/// <exception cref="NoInputsError"><paramref name="inputs"/> is empty.</exception>
		/// <exception cref="OutputConflictError">
		/// <c>--out-file</c> with multiple inputs and no <c>--combine</c>, or two
		/// inputs resolving to the same output path.
		/// </exception>
		public static RunPlan Plan(SharedOptions shared, ConvertOptions convert, IReadOnlyList<InputSource> inputs,
								   IFileSystem fileSystem)
		{
			if (inputs.Count == 0)
				throw new NoInputsError("no inputs matched; the walk and filters selected nothing");

			var plan = new RunPlan
			{
				Format = convert.Format,
				Cardinality = ResolveCardinality(shared, convert, inputs.Count)
			};

            if (plan.Cardinality == OutputCardinality.Combined)
                plan.CombinedTarget = shared.Output.OutFile;

            FormatSpec format = convert.Format;
			OutputNameTemplate template = shared.Output.NameTemplate ?? OutputNameTemplate.Default(format.IsSingleFile);
			string outDir = shared.Output.OutDir ?? ".";
			PageRange pages = convert.Pages ?? PageRange.All;
			foreach (InputSource input in inputs)
			{
				var item = new PlannedItem
				{
					SourcePath = input.Path,
					DisplayName = input.Path,
					Pages = pages
				};

				switch (plan.Cardinality)
				{
					case OutputCardinality.PerInput:
						AssignPerInputTarget(item, input, format, template, outDir);
						break;

					case OutputCardinality.SingleTarget:
						item.Target = shared.Output.OutFile;
						break;

					case OutputCardinality.Combined:
					case OutputCardinality.Stream:
						// Neither owns a path of its own: a combined item feeds
						// RunPlan.CombinedTarget, and a streamed one writes to stdout.
						break;
				}

				plan.Items.Add(item);
			}

			CheckForCollisions(plan, fileSystem);
			CheckOutputsAreNotInputs(plan, fileSystem);

			return plan;
		}

		/// <summary>
		/// Resolves one <see cref="OutputCardinality.PerInput"/> item's target: the
		/// source tree replicated under <c>--out-dir</c>, then the naming template
		/// applied to the filename only.
		/// </summary>
		private static void AssignPerInputTarget(PlannedItem item, InputSource input, FormatSpec format, OutputNameTemplate template, string outDir)
		{
			string relativeDirectory = DirectoryOf(input.RelativePath ?? input.Path);
			string directory = CombineDirectories(outDir, relativeDirectory);

			if (format.IsSingleFile)
			{
				item.Target = JoinPath(directory, template.Render(item.SourcePath, format.Extension, 1));
			}
			else
			{
				item.TargetPattern = new TargetPattern
				{
					Directory = directory,
					Template = template,
					Extension = format.Extension
				};
			}
		}

		/// <summary>The directory portion of a portable, forward-slash relative path.</summary>
		private static string DirectoryOf(string relativePath)
		{
			string normalised = relativePath.Replace('\\', '/');
			int slash = normalised.LastIndexOf('/');
			if (slash < 0)
				return string.Empty;
			return normalised.Substring(0, slash);
		}

		private static string CombineDirectories(string outDir, string relativeDirectory)
		{
			if (string.IsNullOrEmpty(relativeDirectory))
				return outDir;

			if (string.IsNullOrEmpty(outDir))
				return relativeDirectory;
			return outDir + "/" + relativeDirectory;
		}

		private static string JoinPath(string directory, string fileName)
		{
			return string.IsNullOrEmpty(directory) ? fileName : (directory + "/" + fileName);
		}

		/// <summary>
		/// The one part of planning that has to touch the filesystem: refusing to
		/// clobber existing files without <c>--overwrite</c>. Kept separate so that
		/// <see cref="Plan"/> itself stays pure.
		/// </summary>
		/// <remarks>
		/// For a single-file item this is the whole check. For a per-page item it
		/// checks only <see cref="TargetPattern.FirstTarget"/>, because the remaining
		/// names depend on the document's page count; execution checks each page as it
		/// goes, through <see cref="VerifyPageTarget"/>.
		/// </remarks>
		/// <exception cref="OutputConflictError">A target exists and <c>--overwrite</c> was not given.</exception>
		public static void VerifyTargets(RunPlan plan, SharedOptions shared, IFileSystem fileSystem)
        {
            if (shared.Output.Overwrite)
                return;

            if (plan.Cardinality == OutputCardinality.Combined)
			{
				VerifyPageTarget(plan.CombinedTarget, shared, fileSystem);
			}
			else
			{
				foreach (PlannedItem item in plan.Items)
				{
					// A Stream item owns neither: it writes to stdout, not a path.
					if (item.Target == null && item.TargetPattern == null)
						continue;

                    VerifyPageTarget(item.IsPageCountDependent ? item.TargetPattern.FirstTarget(item.SourcePath, item.Pages) : item.Target,
									 shared, fileSystem);
				}
			}
		}

		/// <summary>
		/// The deferred half of <see cref="VerifyTargets"/>, called during execution
		/// once a page-count-dependent item's real filenames are known.
		/// </summary>
		public static void VerifyPageTarget(string path, SharedOptions shared, IFileSystem fileSystem)
        {
            if (shared.Output.Overwrite)
                return;

            if (fileSystem.FileExists(path))
				throw new OutputConflictError(ErrorCodes.OutputExists, $"{path} already exists; pass --overwrite to replace it");
		}

		/// <summary>Chooses the cardinality from the output arguments and the input count.</summary>
		internal static OutputCardinality ResolveCardinality(SharedOptions shared, ConvertOptions convert, int inputCount)
		{
			return ResolveCardinality(shared, inputCount, convert.Combine);
		}

		/// <summary>
		/// The same resolution, for a command with an output group but no
		/// <c>--combine</c> of its own — extract, list and meta, which always pass
		/// <c>combine: false</c>. Their output names come from the container once it
		/// is open, so they stop at cardinality rather than resolving a target.
		/// </summary>
		public static OutputCardinality ResolveCardinality(SharedOptions shared, int inputCount, bool combine)
		{
			if (shared.Output.UseStdout)
				return OutputCardinality.Stream;

			if (shared.Output.OutFile != null)
			{
				if (combine)
					return OutputCardinality.Combined;

				if (inputCount > 1)
				{
					throw new OutputConflictError(ErrorCodes.OutFileMultiInput,
						"--out-file takes a single input; use --combine to merge several into one document");
                }

				return OutputCardinality.SingleTarget;
			}

			return OutputCardinality.PerInput;
		}

		/// <summary>
		/// Detects two inputs resolving to the same output path, which happens when a
		/// flattened walk contains two files with the same stem.
		/// </summary>
		/// <remarks>
		/// For statically resolved items this compares the target paths directly. For
		/// page-count-dependent items it compares
		/// <see cref="TargetPattern.CollisionKey"/>, which catches the real-world case
		/// — <c>a/report.docx</c> and <c>b/report.docx</c> both flattening into one
		/// output directory — without needing to know how many pages either has.
		/// </remarks>
		internal static void CheckForCollisions(RunPlan plan, IFileSystem fileSystem)
		{
			var seen = new HashSet<string>(StringComparer.Ordinal);
			foreach (PlannedItem item in plan.Items)
			{
				string key = item.Target ?? item.TargetPattern?.CollisionKey(item.SourcePath);
				if (key != null && !seen.Add(CollisionKeyFor(key, fileSystem)))
				{
					throw new OutputConflictError(ErrorCodes.OutputCollision,
						"two inputs would write to the same output; use --out-name to distinguish them");
				}
			}
		}

		/// <summary>
		/// Refuses a plan that would write over one of the documents it reads.
		/// </summary>
		/// <remarks>
		/// <c>--overwrite</c> says an existing file may be replaced, and an input is an
		/// existing file, so nothing else here stops <c>convert --to text -o . --overwrite
		/// notes.txt</c> from naming <c>notes.txt</c> as both. The combined case is the
		/// plainest: the output is created, and therefore truncated, before the first
		/// document is opened, so the source is gone and the run then fails to read it.
		/// A glob reaches the same place without a typo — a second run over a directory
		/// picks up the first run's output as an input.
		///
		/// This is the collision check pointed at the other set: two outputs must not
		/// share a path, and an output must not share one with an input either. It uses
		/// the same key, so it folds case exactly where the filesystem does.
		///
		/// A page-count-dependent item is compared by its first target, for the reason
		/// <see cref="VerifyTargets"/> gives: the rest of the names are not known until
		/// the document is open. Page one is the one that would be written first, and
		/// writing it is what would destroy the source.
		/// </remarks>
		internal static void CheckOutputsAreNotInputs(RunPlan plan, IFileSystem fileSystem)
		{
			var inputs = new HashSet<string>(StringComparer.Ordinal);
			foreach (PlannedItem item in plan.Items)
				inputs.Add(CollisionKeyFor(item.SourcePath, fileSystem));

			foreach (string target in PlannedTargets(plan))
			{
				if (inputs.Contains(CollisionKeyFor(target, fileSystem)))
				{
					throw new OutputConflictError(ErrorCodes.OutputIsInput,
						target + " is also an input; use --out-dir or --out-name to write somewhere else");
				}
			}
		}

		/// <summary>Every path the plan would write, as far as it is knowable before the run.</summary>
		private static IEnumerable<string> PlannedTargets(RunPlan plan)
		{
			if (plan.CombinedTarget != null)
				yield return plan.CombinedTarget;

			foreach (PlannedItem item in plan.Items)
			{
				if (item.Target != null)
					yield return item.Target;
				else if (item.TargetPattern != null)
					yield return item.TargetPattern.FirstTarget(item.SourcePath, item.Pages);
			}
		}

		/// <summary>
		/// A planned path reduced to what two outputs must not share.
		/// </summary>
		/// <remarks>
		/// The path itself where the filesystem tells names apart by case, and a
		/// case-folded copy where it does not. Comparing exactly everywhere let
		/// <c>a/Report.docx</c> and <c>b/report.docx</c> both pass planning and then
		/// write to one file on Windows and macOS: the run exited 0, reported nothing,
		/// and the second output had silently replaced the first.
		///
		/// Folding everywhere instead would have been the other error — on a
		/// case-sensitive filesystem those are two files, and refusing to write both
		/// declines work the tool can do correctly. So the question goes to
		/// <see cref="IFileSystem.IsCaseSensitive"/> rather than to the host or to a
		/// fixed rule, which is also what keeps this testable: a fake filesystem
		/// states its own answer, so both behaviours are covered on every platform
		/// instead of only the one the suite happens to run on.
		///
		/// ASCII only, matching <see cref="Configuration.ValueParsers.FoldToken"/>'s
		/// reasoning rather than reusing it — that one is for enumerated values and
		/// says paths are taken as typed. A locale-aware fold would make collision
		/// detection depend on the machine's locale; a Unicode-aware one would not be
		/// reproducible in all four languages without a shared table. Two names
		/// differing only outside ASCII therefore do not collide, in any of them.
		///
		/// What that costs, measured on NTFS: the two spellings of Å are one file
		/// there, so <c>a/Å.docx</c> and <c>b/å.docx</c> flattened into one output
		/// directory pass this check and then write to the same place. What stops the
		/// loss is no longer this key but the moment of the question: each target is
		/// looked at again immediately before it is written, when the earlier output
		/// is on disk and the filesystem — the only authority on whether two names
		/// are one file — answers for itself. The run then fails with
		/// <c>E_OUTPUT_EXISTS</c> rather than silently keeping the second.
		///
		/// Under <c>--overwrite</c> it still keeps the second, because that is what
		/// <c>--overwrite</c> asks for and nothing left can tell this apart from an
		/// ordinary replacement. A pair this key does fold is refused even then.
		/// </remarks>
		public static string CollisionKeyFor(string path, IFileSystem fileSystem)
		{
			if (fileSystem.IsCaseSensitive(path))
				return path;

			char[] chars = path.ToCharArray();
			for (int i = 0; i < chars.Length; i++)
			{
				if (chars[i] >= 'A' && chars[i] <= 'Z')
					chars[i] = (char)(chars[i] - 'A' + 'a');
			}
			return new string(chars);
		}
	}
}
