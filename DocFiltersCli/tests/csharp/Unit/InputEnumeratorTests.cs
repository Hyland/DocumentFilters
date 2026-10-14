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

using System.Collections.Generic;
using System.Linq;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Discovery;
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The filesystem walk: which files a command line actually selects.
	/// </summary>
	/// <remarks>
	/// Runs entirely against <see cref="InMemoryFileSystem"/>, so these are ordinary
	/// fast unit tests despite being about the filesystem. Directory depth here is
	/// entirely separate from subfile depth, which is recursion into documents
	/// nested inside other documents.
	/// </remarks>
	public class InputEnumeratorTests
	{
		private static InMemoryFileSystem Tree()
		{
			return new InMemoryFileSystem()
				.AddFile("corpus/a.docx")
				.AddFile("corpus/b.pdf")
				.AddFile("corpus/reports/q1.docx")
				.AddFile("corpus/reports/q2.docx")
				.AddFile("corpus/reports/archive/old.docx")
				.AddFile("corpus/notes.txt");
		}

		private static IReadOnlyList<string> Walk(InMemoryFileSystem fs, InputOptions options)
		{
			var enumerator = new InputEnumerator(fs, new RecordingConsole());
			return enumerator.Enumerate(options).Select(i => i.Path).ToList();
		}

		private static InputOptions From(params string[] paths)
		{
			var options = new InputOptions();
			options.Paths.AddRange(paths);
			return options;
		}

		// ------------------------------------------------------------------ depth

		[Fact]
		public void A_directory_is_walked_recursively_by_default()
		{
			// Matching fd and ripgrep rather than grep and cp: a directory handed to a
			// batch converter is a corpus to walk.
			IReadOnlyList<string> found = Walk(Tree(), From("corpus"));

			Assert.Contains("corpus/a.docx", found);
			Assert.Contains("corpus/reports/q1.docx", found);
			Assert.Contains("corpus/reports/archive/old.docx", found);
		}

		[Fact]
		public void No_recurse_stays_in_the_named_directory()
		{
			InputOptions options = From("corpus");
			options.NoRecurseWasGiven = true;

			IReadOnlyList<string> found = Walk(Tree(), options);

			Assert.Contains("corpus/a.docx", found);
			Assert.DoesNotContain("corpus/reports/q1.docx", found);
		}

		[Fact]
		public void Max_depth_one_is_the_same_as_no_recurse()
		{
			InputOptions options = From("corpus");
			options.MaxDepth = 1;

			Assert.DoesNotContain("corpus/reports/q1.docx", Walk(Tree(), options));
		}

		[Fact]
		public void Max_depth_two_reaches_one_level_of_subdirectory()
		{
			InputOptions options = From("corpus");
			options.MaxDepth = 2;

			IReadOnlyList<string> found = Walk(Tree(), options);

			Assert.Contains("corpus/reports/q1.docx", found);
			Assert.DoesNotContain("corpus/reports/archive/old.docx", found);
		}

		// ------------------------------------------------------------ explicit files

		[Fact]
		public void An_explicit_file_is_taken_as_given()
		{
			Assert.Equal(new[] { "corpus/a.docx" }, Walk(Tree(), From("corpus/a.docx")));
		}

		[Fact]
		public void Files_and_directories_can_be_mixed_and_keep_their_order()
		{
			IReadOnlyList<string> found = Walk(Tree(), From("corpus/notes.txt", "corpus/reports"));

			Assert.Equal("corpus/notes.txt", found[0]);
			Assert.Contains("corpus/reports/q1.docx", found);
		}

		[Fact]
		public void An_explicit_file_is_not_subject_to_the_name_matcher()
		{
			// Naming a file directly is an instruction, not a suggestion; a glob filters
			// what a walk discovers.
			InputOptions options = From("corpus/notes.txt");
			options.Matcher = NameMatcher.FromGlobs(new[] { "**/*.docx" });

			Assert.Equal(new[] { "corpus/notes.txt" }, Walk(Tree(), options));
		}

		// ---------------------------------------------------------------- selection

		[Fact]
		public void A_glob_filters_what_the_walk_discovers()
		{
			InputOptions options = From("corpus");
			options.Matcher = NameMatcher.FromGlobs(new[] { "**/*.docx" });

			IReadOnlyList<string> found = Walk(Tree(), options);

			Assert.Contains("corpus/a.docx", found);
			Assert.DoesNotContain("corpus/b.pdf", found);
			Assert.DoesNotContain("corpus/notes.txt", found);
		}

		[Fact]
		public void An_exclusion_removes_a_subtree()
		{
			InputOptions options = From("corpus");
			options.Matcher = NameMatcher.FromGlobs(new[] { "**/*.docx", "!**/archive/*" });

			IReadOnlyList<string> found = Walk(Tree(), options);

			Assert.Contains("corpus/reports/q1.docx", found);
			Assert.DoesNotContain("corpus/reports/archive/old.docx", found);
		}

		// ------------------------------------------------------------------ hidden

		[Fact]
		public void Hidden_entries_are_skipped_by_default()
		{
			InMemoryFileSystem fs = Tree().AddFile("corpus/.secret.docx").MarkHidden("corpus/.secret.docx");

			Assert.DoesNotContain("corpus/.secret.docx", Walk(fs, From("corpus")));
		}

		[Fact]
		public void Include_hidden_brings_them_back()
		{
			InMemoryFileSystem fs = Tree().AddFile("corpus/.secret.docx").MarkHidden("corpus/.secret.docx");
			InputOptions options = From("corpus");
			options.IncludeHidden = true;

			Assert.Contains("corpus/.secret.docx", Walk(fs, options));
		}

		[Fact]
		public void A_hidden_directory_hides_everything_under_it()
		{
			InMemoryFileSystem fs = Tree().AddFile("corpus/.git/config.docx").MarkHidden("corpus/.git");

			Assert.DoesNotContain("corpus/.git/config.docx", Walk(fs, From("corpus")));
		}

		// ---------------------------------------------------------------- symlinks

		[Fact]
		public void A_symlinked_directory_is_not_entered_by_default()
		{
			// Following by default risks walking in a circle.
			InMemoryFileSystem fs = Tree().AddFile("corpus/link/x.docx").MarkSymlink("corpus/link");

			Assert.DoesNotContain("corpus/link/x.docx", Walk(fs, From("corpus")));
		}

		[Fact]
		public void Follow_enters_symlinked_directories()
		{
			InMemoryFileSystem fs = Tree().AddFile("corpus/link/x.docx").MarkSymlink("corpus/link");
			InputOptions options = From("corpus");
			options.FollowSymlinks = true;

			Assert.Contains("corpus/link/x.docx", Walk(fs, options));
		}

		[Fact]
		public void Follow_terminates_when_a_link_points_at_an_ancestor()
		{
			// The reason the walk tracks directory identity rather than path strings: a
			// link back to its own parent produces an endless supply of distinct paths
			// — corpus/loop/loop/loop/… — so a visited-paths set never closes the cycle.
			InMemoryFileSystem fs = Tree().MarkSymlink("corpus/loop", "corpus");
			InputOptions options = From("corpus");
			options.FollowSymlinks = true;

			IReadOnlyList<string> found = Walk(fs, options);

			Assert.DoesNotContain(found, p => p.Contains("loop/loop"));
		}

		[Fact]
		public void Follow_visits_a_shared_directory_once()
		{
			// Two links to the same place is not a cycle, but it is still one directory:
			// processing it twice would convert every document in it twice.
			InMemoryFileSystem fs = Tree()
				.AddFile("shared/x.docx")
				.MarkSymlink("corpus/one", "shared")
				.MarkSymlink("corpus/two", "shared");

			InputOptions options = From("corpus");
			options.FollowSymlinks = true;

			IReadOnlyList<string> found = Walk(fs, options);

			Assert.Equal(1, found.Count(p => p.EndsWith("x.docx", System.StringComparison.Ordinal)));
		}

		// -------------------------------------------------------------------- sort

		[Fact]
		public void Sort_orders_the_walk()
		{
			InputOptions options = From("corpus");
			options.Sort = true;

			IReadOnlyList<string> found = Walk(Tree(), options);

			Assert.Equal(found.OrderBy(p => p, System.StringComparer.Ordinal).ToList(), found);
		}

		[Fact]
		public void Without_sort_the_walk_follows_the_filesystem_order()
		{
			// The counterpart that makes the test above meaningful: the fake returns
			// entries unordered, so if --sort were ignored the sorted assertion would
			// still have to fail somewhere.
			IReadOnlyList<string> found = Walk(Tree(), From("corpus"));

			Assert.NotEqual(found.OrderBy(p => p, System.StringComparer.Ordinal).ToList(), found);
		}

		// -------------------------------------------------------------- files-from

		[Fact]
		public void Files_from_reads_a_list()
		{
			InMemoryFileSystem fs = Tree().AddFile("list.txt", "corpus/a.docx\ncorpus/b.pdf\n");
			InputOptions options = new InputOptions { FilesFrom = "list.txt" };

			Assert.Equal(new[] { "corpus/a.docx", "corpus/b.pdf" }, Walk(fs, options));
		}

		[Fact]
		public void Files_from_ignores_blank_lines()
		{
			InMemoryFileSystem fs = Tree().AddFile("list.txt", "corpus/a.docx\n\n\ncorpus/b.pdf\n");
			InputOptions options = new InputOptions { FilesFrom = "list.txt" };

			Assert.Equal(new[] { "corpus/a.docx", "corpus/b.pdf" }, Walk(fs, options));
		}

		[Fact]
		public void Files_from_dash_reads_standard_input()
		{
			// "-" is not a path, so it goes through the console seam rather than the
			// filesystem — otherwise a test could not supply it.
			var console = new RecordingConsole();
			console.InputLines.Add("corpus/a.docx");
			console.InputLines.Add("corpus/b.pdf");

			var enumerator = new InputEnumerator(Tree(), console);
			IReadOnlyList<InputSource> found = enumerator.Enumerate(new InputOptions { FilesFrom = "-" });

			Assert.Equal(new[] { "corpus/a.docx", "corpus/b.pdf" }, found.Select(i => i.Path));
		}

		[Fact]
		public void Files_from_combines_with_positional_paths()
		{
			InMemoryFileSystem fs = Tree().AddFile("list.txt", "corpus/b.pdf\n");
			InputOptions options = From("corpus/a.docx");
			options.FilesFrom = "list.txt";

			IReadOnlyList<string> found = Walk(fs, options);

			Assert.Contains("corpus/a.docx", found);
			Assert.Contains("corpus/b.pdf", found);
		}

		// ------------------------------------------------------- relative paths

		[Fact]
		public void A_walked_file_records_its_path_relative_to_the_root()
		{
			// This is what lets --out-dir replicate the source tree.
			var enumerator = new InputEnumerator(Tree(), new RecordingConsole());

			InputSource found = enumerator.Enumerate(From("corpus"))
				.Single(i => i.Path == "corpus/reports/q1.docx");

			Assert.Equal("reports/q1.docx", found.RelativePath);
			Assert.Equal("corpus", found.Root);
		}

		[Fact]
		public void An_explicit_file_has_no_root_and_is_relative_to_itself()
		{
			var enumerator = new InputEnumerator(Tree(), new RecordingConsole());

			InputSource found = enumerator.Enumerate(From("corpus/a.docx")).Single();

			Assert.Equal("a.docx", found.RelativePath);
			Assert.Null(found.Root);
		}

		[Fact]
		public void A_backslash_path_still_reduces_to_a_bare_filename()
		{
			// A directory walk always yields forward-slash paths (WalkInto builds them
			// from IFileSystem.ListDirectory), but an explicit argument is whatever the
			// user typed on their own shell — backslashes on Windows. Reproduces the
			// real bug: "C:\tmp\a.pdf" was kept whole as RelativePath because the old
			// FileNameOf only recognised '/', so --out-dir nested the source drive
			// underneath itself instead of flattening to "a.pdf".
			var enumerator = new InputEnumerator(Tree(), new RecordingConsole());

			InputSource found = enumerator.Enumerate(From(@"corpus\a.docx")).Single();

			Assert.Equal("a.docx", found.RelativePath);
			Assert.Null(found.Root);
		}

		// ---------------------------------------------------------------- nothing

		[Fact]
		public void Matching_nothing_is_its_own_error()
		{
			InputOptions options = From("corpus");
			options.Matcher = NameMatcher.FromGlobs(new[] { "**/*.nonexistent" });

			NoInputsError error = Assert.Throws<NoInputsError>(() => Walk(Tree(), options));

			Assert.Equal(ExitCode.NoInputsMatched, error.ExitCode);
		}

		[Fact]
		public void An_empty_directory_matches_nothing()
		{
			var fs = new InMemoryFileSystem().AddDirectory("empty");

			Assert.Throws<NoInputsError>(() => Walk(fs, From("empty")));
		}

		[Fact]
		public void A_path_that_does_not_exist_is_an_error()
		{
			Assert.ThrowsAny<CliError>(() => Walk(Tree(), From("corpus/missing.docx")));
		}
	}

	/// <summary>
	/// The in-memory filesystem itself. Test infrastructure needs testing too: a
	/// fake that silently discards writes would make every conversion test above it
	/// meaningless.
	/// </summary>
	public class InMemoryFileSystemTests
	{
		[Fact]
		public void A_created_file_keeps_what_was_written_to_it()
		{
			var fs = new InMemoryFileSystem();

			using (var writer = new System.IO.StreamWriter(fs.CreateFile("out/report.txt")))
				writer.Write("hello");

			Assert.Equal("hello", fs.ReadText("out/report.txt"));
		}

		[Fact]
		public void A_created_file_can_be_read_back_through_the_interface()
		{
			var fs = new InMemoryFileSystem();

			using (var writer = new System.IO.StreamWriter(fs.CreateFile("out/report.txt")))
				writer.Write("hello");

			using (var reader = new System.IO.StreamReader(fs.OpenRead("out/report.txt")))
				Assert.Equal("hello", reader.ReadToEnd());
		}

		[Fact]
		public void Creating_a_file_creates_its_parent_directories()
		{
			var fs = new InMemoryFileSystem();

			fs.CreateFile("out/deep/nested/report.txt").Dispose();

			Assert.True(fs.DirectoryExists("out/deep/nested"));
			Assert.True(fs.FileExists("out/deep/nested/report.txt"));
		}

		[Fact]
		public void Listing_returns_immediate_children_only()
		{
			var fs = new InMemoryFileSystem()
				.AddFile("a/b.txt")
				.AddFile("a/c/d.txt");

			// As a set: the fake returns children in a deliberately unhelpful order so
			// that a walk which ignores --sort cannot pass the sorting tests by
			// accident. Asserting a sequence here would be asserting that anti-feature
			// rather than the property this test is named for.
			Assert.Equal(
				new[] { "a/b.txt", "a/c" }.OrderBy(p => p, StringComparer.Ordinal),
				fs.ListDirectory("a").OrderBy(p => p, StringComparer.Ordinal));
		}

		[Fact]
		public void Paths_are_normalised_so_tests_read_the_same_on_every_platform()
		{
			var fs = new InMemoryFileSystem().AddFile(@"a\b.txt");

			Assert.True(fs.FileExists("a/b.txt"));
		}

		[Fact]
		public void A_link_is_a_real_directory_entry_so_a_walk_can_find_it()
		{
			// Without this the walk never sees the link, and every test about following
			// one passes without exercising anything.
			var fs = new InMemoryFileSystem().AddFile("corpus/a.txt").MarkSymlink("corpus/link", "target");

			Assert.Contains("corpus/link", fs.ListDirectory("corpus"));
			Assert.True(fs.IsSymbolicLink("corpus/link"));
		}

		[Fact]
		public void Listing_through_a_link_yields_the_targets_children_under_the_link_path()
		{
			var fs = new InMemoryFileSystem()
				.AddFile("shared/x.docx")
				.MarkSymlink("corpus/link", "shared");

			Assert.Equal(new[] { "corpus/link/x.docx" }, fs.ListDirectory("corpus/link"));
		}

		[Fact]
		public void A_link_to_an_ancestor_produces_an_endless_supply_of_distinct_paths()
		{
			// The property that makes path-based cycle detection fail, reproduced in the
			// fake so the walk's identity-based detection has something real to solve.
			var fs = new InMemoryFileSystem().AddFile("corpus/a.txt").MarkSymlink("corpus/loop", "corpus");

			Assert.Contains("corpus/loop/a.txt", fs.ListDirectory("corpus/loop"));
			Assert.Contains("corpus/loop/loop", fs.ListDirectory("corpus/loop"));
			Assert.Contains("corpus/loop/loop/loop", fs.ListDirectory("corpus/loop/loop"));
		}

		[Fact]
		public void Two_links_to_one_directory_share_an_identity()
		{
			var fs = new InMemoryFileSystem()
				.AddFile("shared/x.docx")
				.MarkSymlink("corpus/one", "shared")
				.MarkSymlink("corpus/two", "shared");

			Assert.Equal(fs.GetDirectoryIdentity("corpus/one"), fs.GetDirectoryIdentity("corpus/two"));
			Assert.NotEqual(fs.GetDirectoryIdentity("corpus/one"), fs.GetDirectoryIdentity("corpus"));
		}

		/// <summary>
		/// A directory the walk cannot read stops the run and says so.
		/// </summary>
		/// <remarks>
		/// The alternative — carrying on with whatever could be listed — hands back a
		/// short input list that looks complete, so a run silently converts fewer
		/// documents than it was given. The C++ port did exactly that by ignoring the
		/// error_code its listing reported; this one let the platform's exception reach
		/// the top and end the process. Neither told anyone a directory had been
		/// skipped.
		///
		/// The message is fixed rather than the platform's own, which differs between
		/// .NET and libstdc++ and would not be the same contract in two languages.
		/// </remarks>
		[Fact]
		public void A_directory_that_cannot_be_read_is_reported_rather_than_skipped()
		{
			var fileSystem = new InMemoryFileSystem();
			fileSystem.AddFile("corpus/a.docx");
			fileSystem.AddFile("corpus/locked/b.docx");
			fileSystem.Unreadable.Add("corpus/locked");

			var enumerator = new InputEnumerator(fileSystem, new RecordingConsole());

			NoInputsError error = Assert.Throws<NoInputsError>(
				() => enumerator.Enumerate(new InputOptions { Paths = { "corpus" } }));

			Assert.Equal("corpus/locked: could not be read", error.Message);
			Assert.Equal(ExitCode.NoInputsMatched, error.ExitCode);
		}
	}
}
