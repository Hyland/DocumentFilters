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
using Hyland.DocumentFilters.Cli.Execution;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// Where <c>DocumentFiltersSession.Create</c> looks for the engine's own
	/// resources — fonts included.
	/// </summary>
	/// <remarks>
	/// doc2text and the other published samples default this to <c>"."</c> and get
	/// away with it because they always run from their own build output directory.
	/// This tool is invoked from wherever the user's documents happen to be, so
	/// <c>"."</c> there resolves to a directory with no fonts at all: the engine
	/// still initialises, but falls back to a font it cannot rasterise glyphs from,
	/// so rendered text comes out blank instead of missing. <c>ResolveLibraryPath</c>
	/// is the precedence decision alone, pure and without the environment or the
	/// real executable path, so it is testable without either.
	/// </remarks>
	public class DocumentFiltersSessionTests
	{
		[Fact]
		public void An_explicit_library_path_wins_over_everything()
		{
			var options = new LibraryOptions { LibraryPath = "/opt/df" };

			string resolved = DocumentFiltersSession.ResolveLibraryPath(options, "/env/df", "/exe/dir");

			Assert.Equal("/opt/df", resolved);
		}

		[Fact]
		public void The_environment_variable_wins_over_the_executable_directory()
		{
			var options = new LibraryOptions();

			string resolved = DocumentFiltersSession.ResolveLibraryPath(options, "/env/df", "/exe/dir");

			Assert.Equal("/env/df", resolved);
		}

		[Fact]
		public void With_neither_argument_nor_environment_variable_it_falls_back_to_the_executable_directory()
		{
			// Not the process's current directory: a general-purpose CLI is invoked
			// from wherever the user's documents are, not from its own install
			// directory, and "." would send font lookup there instead.
			var options = new LibraryOptions();

			string resolved = DocumentFiltersSession.ResolveLibraryPath(options, null, "/exe/dir");

			Assert.Equal("/exe/dir", resolved);
		}
	}
}
