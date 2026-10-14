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

using System;
using System.Collections.Generic;
using System.Linq;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Engine;
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// What the subfile walk does with each way a container can misbehave.
	/// </summary>
	/// <remarks>
	/// The engine hands subfile names straight out of the container, which is
	/// attacker-controlled data in a way a file on disk is not. These check both
	/// halves: that a failure inside a container is handled, and that a hostile name
	/// cannot write outside the output directory (D17).
	/// </remarks>
	public class EngineSubfileOutcomeTests
	{
		/// <summary>
		/// Every failure the engine can report of its own accord.
		/// </summary>
		/// <remarks>
		/// <see cref="EngineError.None"/> is not a failure, and
		/// <see cref="EngineError.Cancelled"/> is not the engine's: it is only ever
		/// raised because this tool asked, through the open callback, so the caller
		/// that asked translates it into its own error — a <c>--timeout</c> reported as
		/// <c>E_LIMIT_EXCEEDED</c> rather than a document reported as broken. Sweeping
		/// it here would assert the opposite.
		/// </remarks>
		public static IEnumerable<object[]> EveryEngineError()
		{
			foreach (EngineError error in Enum.GetValues(typeof(EngineError)).Cast<EngineError>())
			{
				if (error != EngineError.None && error != EngineError.Cancelled)
					yield return new object[] { error };
			}
		}

		private static SharedOptions Shared(bool subfilesEnabled = false, int? depth = null)
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";
			shared.Subfiles.Enabled = subfilesEnabled;
			shared.Subfiles.MaxDepth = depth;
			return shared;
		}

		// ============================================ listing

		[Fact]
		public void Every_entry_the_container_reports_is_listed()
		{
			FakeDocument container = new FakeDocument("bundle.zip");
			container.SubFileEntries.Add(new FakeSubFile("readme.txt", "0001") { Size = 4096, Modified = new DateTime(2026, 1, 15, 9, 30, 0) });
			container.SubFileEntries.Add(new FakeSubFile("reports/q1.docx", "0002") { Size = 20480 });

			IReadOnlyList<SubfileEntry> entries = new SubfileExtraction(new InMemoryFileSystem()).ListAll(container, "bundle.zip", Shared());

			Assert.Equal(2, entries.Count);
			Assert.Equal("bundle.zip>readme.txt", entries[0].DisplayName);
			Assert.Equal("2026-01-15T09:30:00Z", entries[0].Modified);
			Assert.Equal(1, entries[0].Depth);
		}

		[Fact]
		public void A_container_with_no_entries_lists_nothing_rather_than_failing()
		{
			IReadOnlyList<SubfileEntry> entries = new SubfileExtraction(new InMemoryFileSystem())
				.ListAll(new FakeDocument("flat.txt"), "flat.txt", Shared());

			Assert.Empty(entries);
		}

		[Theory]
		[MemberData(nameof(EveryEngineError))]
		public void A_container_that_fails_to_enumerate_surfaces_the_engine_error(EngineError error)
		{
			// Nothing above the seam catches IGRException, so this has to arrive as an
			// EngineException for the command layer to turn into a DocumentError.
			var container = new FakeDocument("bundle.zip") { ReadFailure = error, ReadFailureOn = "SubFiles" };

			EngineException thrown = Assert.Throws<EngineException>(
				() => new SubfileExtraction(new InMemoryFileSystem()).ListAll(container, "bundle.zip", Shared()));

			Assert.Equal(error, thrown.Error);
			Assert.Equal((int)error, thrown.Code);
		}

		// ============================================ nesting

		[Fact]
		public void Nested_containers_are_walked_when_subfiles_are_enabled()
		{
			var inner = new FakeDocument("inner.zip");
			inner.SubFileEntries.Add(new FakeSubFile("note.txt", "0001"));

			var outer = new FakeDocument("nested.zip");
			outer.SubFileEntries.Add(new FakeSubFile("inner.zip", "0001") { Content = inner });

			IReadOnlyList<SubfileEntry> entries = new SubfileExtraction(new InMemoryFileSystem())
				.ListAll(outer, "nested.zip", Shared(subfilesEnabled: true));

			Assert.Equal(2, entries.Count);
			Assert.Equal("nested.zip>inner.zip", entries[0].DisplayName);
			Assert.Equal("nested.zip>inner.zip>note.txt", entries[1].DisplayName);
			Assert.Equal(2, entries[1].Depth);
		}

		[Fact]
		public void Subfile_depth_stops_the_walk_where_it_says()
		{
			var deepest = new FakeDocument("deepest.zip");
			deepest.SubFileEntries.Add(new FakeSubFile("buried.txt", "0001"));

			var middle = new FakeDocument("middle.zip");
			middle.SubFileEntries.Add(new FakeSubFile("deepest.zip", "0001") { Content = deepest });

			var outer = new FakeDocument("outer.zip");
			outer.SubFileEntries.Add(new FakeSubFile("middle.zip", "0001") { Content = middle });

			IReadOnlyList<SubfileEntry> entries = new SubfileExtraction(new InMemoryFileSystem())
				.ListAll(outer, "outer.zip", Shared(subfilesEnabled: true, depth: 2));

			// Depth 2 reaches middle.zip's own children and stops before their contents.
			Assert.Equal(2, entries.Count);
			Assert.DoesNotContain(entries, entry => entry.Name == "buried.txt");
		}

		[Fact]
		public void Without_subfiles_enabled_only_the_top_level_is_reported()
		{
			var inner = new FakeDocument("inner.zip");
			inner.SubFileEntries.Add(new FakeSubFile("note.txt", "0001"));

			var outer = new FakeDocument("nested.zip");
			outer.SubFileEntries.Add(new FakeSubFile("inner.zip", "0001") { Content = inner });

			IReadOnlyList<SubfileEntry> entries = new SubfileExtraction(new InMemoryFileSystem()).ListAll(outer, "nested.zip", Shared());

			Assert.Single(entries);
		}

		[Theory]
		[MemberData(nameof(EveryEngineError))]
		public void An_entry_that_will_not_open_surfaces_the_engine_error(EngineError error)
		{
			var outer = new FakeDocument("nested.zip");
			outer.SubFileEntries.Add(new FakeSubFile("inner.zip", "0001") { OpenFailure = error });

			EngineException thrown = Assert.Throws<EngineException>(
				() => new SubfileExtraction(new InMemoryFileSystem()).ListAll(outer, "nested.zip", Shared(subfilesEnabled: true)));

			Assert.Equal(error, thrown.Error);
		}

		// ============================================ extraction

		[Fact]
		public void Extraction_writes_every_entry_that_passes_the_filters()
		{
			var container = new FakeDocument("archive.zip");
			var keep = new FakeSubFile("keep.docx", "0001");
			var drop = new FakeSubFile("draft-notes.txt", "0002");
			container.SubFileEntries.Add(keep);
			container.SubFileEntries.Add(drop);

			// Compiled the way CommandLineApp compiles them when the argument is read,
			// so this tests the walk's use of a matcher rather than the binding of one.
			var options = new ExtractOptions
			{
				SubfileInclude = Hyland.DocumentFilters.Cli.Planning.NameMatcher.FromGlobs(new[] { "*.docx" })
			};

			IReadOnlyList<string> written = new SubfileExtraction(new InMemoryFileSystem())
				.ExtractAll(container, "archive.zip", Shared(), options);

			Assert.Single(written);
			Assert.Single(keep.CopiedTo);
			Assert.Empty(drop.CopiedTo);
		}

		[Theory]
		[MemberData(nameof(EveryEngineError))]
		public void An_entry_that_will_not_copy_surfaces_the_engine_error(EngineError error)
		{
			var container = new FakeDocument("archive.zip");
			container.SubFileEntries.Add(new FakeSubFile("broken.docx", "0001") { CopyFailure = error });

			EngineException thrown = Assert.Throws<EngineException>(
				() => new SubfileExtraction(new InMemoryFileSystem()).ExtractAll(container, "archive.zip", Shared(), new ExtractOptions()));

			Assert.Equal(error, thrown.Error);
		}

		/// <summary>
		/// A container-supplied name can never write outside the output directory.
		/// </summary>
		/// <remarks>
		/// D17. The name comes from the archive, not from the person running the tool,
		/// so it is attacker-controlled: traversal, an absolute path, a drive letter and
		/// a UNC share all have to be neutralised. Checked through the real extraction
		/// path rather than only against <c>ResolveSafeOutputPath</c>, so that a future
		/// caller bypassing it is caught here.
		/// </remarks>
		[Theory]
		[InlineData("../../outside.txt")]
		[InlineData("..\\..\\outside.txt")]
		[InlineData("/etc/passwd")]
		[InlineData("C:\\Windows\\System32\\evil.dll")]
		[InlineData("\\\\server\\share\\evil.dll")]
		[InlineData("../")]
		[InlineData("..")]
		public void A_hostile_entry_name_stays_inside_the_output_directory(string hostileName)
		{
			var container = new FakeDocument("archive.zip");
			var entry = new FakeSubFile(hostileName, "0001");
			container.SubFileEntries.Add(entry);

			IReadOnlyList<string> written = new SubfileExtraction(new InMemoryFileSystem())
				.ExtractAll(container, "archive.zip", Shared(), new ExtractOptions());

			string target = Assert.Single(written);
			Assert.StartsWith("out/", target, StringComparison.Ordinal);
			Assert.DoesNotContain("..", target);
			Assert.Equal(target, Assert.Single(entry.CopiedTo));
		}

		[Fact]
		public void Images_are_extracted_only_when_asked_for()
		{
			var container = new FakeDocument("doc.docx");
			var image = new FakeSubFile("image1.png", "0001");
			container.ImageEntries.Add(image);

			Assert.Empty(new SubfileExtraction(new InMemoryFileSystem()).ExtractAll(container, "doc.docx", Shared(), new ExtractOptions()));
			Assert.Empty(image.CopiedTo);

			IReadOnlyList<string> withImages = new SubfileExtraction(new InMemoryFileSystem())
				.ExtractAll(container, "doc.docx", Shared(), new ExtractOptions { Images = true });

			Assert.Single(withImages);
			Assert.Single(image.CopiedTo);
		}

		// ============================================ timestamps

		[Fact]
		public void A_container_that_records_no_timestamp_reports_none()
		{
			// The binding gives DateTime.MinValue for an entry with no date; reporting
			// that as a date in year one would be a lie.
			Assert.Null(SubfileExtraction.FormatTimestamp(null));
			Assert.Null(SubfileExtraction.FormatTimestamp(DateTime.MinValue));
			Assert.Equal("2026-01-15T09:30:00Z", SubfileExtraction.FormatTimestamp(new DateTime(2026, 1, 15, 9, 30, 0)));
		}
	}
}
