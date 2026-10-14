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
using Hyland.DocumentFilters.Cli.Execution;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// Containment of extracted subfile names.
	/// </summary>
	/// <remarks>
	/// A subfile's name comes from the container, not from the person running the
	/// tool. An archive can carry <c>../../../etc/passwd</c>, an absolute path, or a
	/// Windows drive letter, and a tool that joins that name onto the output
	/// directory writes wherever the archive says — the vulnerability usually called
	/// zip slip.
	///
	/// Everything here is a pure string operation, so the whole attack surface is
	/// testable without an archive, a document or a filesystem.
	/// </remarks>
	public class SubfileExtractionTests
	{
		private static string Safe(string name, bool replicateTree = true)
		{
			return SubfileExtraction.ResolveSafeOutputPath("/out", name, replicateTree);
		}

		private static void AssertInsideRoot(string resolved)
		{
			string normalised = resolved.Replace('\\', '/');

			Assert.StartsWith("/out/", normalised, StringComparison.Ordinal);

			// Traversal is a path *component* equal to "..", not any occurrence of the
			// two characters. Rejecting the substring would also reject the legitimate
			// filename "..data", which the test below requires to survive.
			foreach (string component in normalised.Split('/'))
			{
				Assert.True(component != "..", $"'{resolved}' contains a traversal component");
				Assert.True(component != ".", $"'{resolved}' contains an unresolved '.' component");
			}
		}

		// ------------------------------------------------------------ ordinary names

		[Fact]
		public void An_ordinary_name_lands_directly_in_the_output_directory()
		{
			Assert.Equal("/out/readme.txt", Safe("readme.txt").Replace('\\', '/'));
		}

		[Fact]
		public void A_nested_name_replicates_its_structure()
		{
			Assert.Equal("/out/reports/q1.docx", Safe("reports/q1.docx").Replace('\\', '/'));
		}

		[Fact]
		public void Flat_mode_keeps_only_the_final_component()
		{
			Assert.Equal("/out/q1.docx", Safe("reports/q1.docx", replicateTree: false).Replace('\\', '/'));
		}

		// ---------------------------------------------------------------- traversal

		[Theory]
		[InlineData("../outside.txt")]
		[InlineData("../../outside.txt")]
		[InlineData("../../../../../../etc/passwd")]
		[InlineData("reports/../../outside.txt")]
		[InlineData("./../outside.txt")]
		[InlineData("a/b/../../../outside.txt")]
		public void A_traversing_name_cannot_escape_the_output_directory(string name)
		{
			AssertInsideRoot(Safe(name));
		}

		[Theory]
		[InlineData(@"..\outside.txt")]
		[InlineData(@"..\..\windows\system32\evil.dll")]
		[InlineData(@"reports\..\..\outside.txt")]
		public void Backslash_traversal_is_caught_too(string name)
		{
			// An archive written on Windows uses backslashes, and a tool that only
			// looked for forward slashes would let those through on Linux.
			AssertInsideRoot(Safe(name));
		}

		[Theory]
		[InlineData("/etc/passwd")]
		[InlineData("/absolute/path.txt")]
		[InlineData(@"\windows\system32\evil.dll")]
		public void An_absolute_name_is_taken_as_relative(string name)
		{
			AssertInsideRoot(Safe(name));
		}

		[Theory]
		[InlineData(@"C:\windows\evil.dll")]
		[InlineData(@"C:/windows/evil.dll")]
		[InlineData(@"\\server\share\evil.dll")]
		public void A_drive_or_share_qualified_name_is_taken_as_relative(string name)
		{
			AssertInsideRoot(Safe(name));
		}

		[Fact]
		public void Traversal_is_neutralised_in_flat_mode_as_well()
		{
			// Flat mode reduces to the final component, which happens to defuse most
			// traversal — but a name that is *only* traversal must not slip through.
			AssertInsideRoot(Safe("../../outside.txt", replicateTree: false));
		}

		// -------------------------------------------------------------- odd names

		[Theory]
		[InlineData("..")]
		[InlineData(".")]
		[InlineData("...")]
		[InlineData("")]
		[InlineData("   ")]
		public void A_degenerate_name_still_produces_a_path_inside_the_root(string name)
		{
			AssertInsideRoot(Safe(name));
		}

		[Fact]
		public void A_name_that_is_only_separators_produces_a_path_inside_the_root()
		{
			AssertInsideRoot(Safe("///"));
		}

		[Fact]
		public void A_directory_named_dot_dot_something_is_not_traversal_and_survives()
		{
			// "..data" is a legitimate filename; only the "." and ".." components are
			// traversal, and an over-eager filter would corrupt valid names.
			Assert.Contains("..data", Safe("..data/x.txt"));
			AssertInsideRoot(Safe("..data/x.txt"));
		}

		[Fact]
		public void Two_subfiles_with_the_same_name_resolve_to_the_same_path()
		{
			// Containment does not deduplicate: overwriting is the extract command's
			// business, through --overwrite. This just records that the guard is not
			// silently renaming things.
			Assert.Equal(Safe("readme.txt"), Safe("readme.txt"));
		}
	}
}
