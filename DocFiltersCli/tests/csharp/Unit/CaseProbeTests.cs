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
using Hyland.DocumentFilters.Cli.Runtime;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// Where the case question is asked, which is the part a real filesystem cannot
	/// check: on one volume every directory answers the same, so a probe aimed at the
	/// parent passes on every machine until someone converts into a mounted volume.
	/// </summary>
	public class CaseProbeTests
	{
		/// <summary>
		/// The whole of the finding this answers: every path the probe asks about lies
		/// inside the directory being measured. Aimed at the parent — which is what it
		/// used to do — an output under a mounted volume is measured against the
		/// filesystem holding the mount point, and two outputs differing only by case
		/// are planned as two files that turn out to be one.
		/// </summary>
		[Fact]
		public void Asks_only_about_names_inside_the_directory()
		{
			var asked = new List<string>();

			CaseProbe.Measure("/mnt/ntfs", new[] { "/mnt/ntfs/Report.txt" }, candidate =>
			{
				asked.Add(candidate);
				return false;
			});

			Assert.Equal(new[] { "/mnt/ntfs/rEPORT.TXT" }, asked);
		}

		[Fact]
		public void Finding_the_flipped_spelling_means_case_insensitive()
		{
			Assert.False(CaseProbe.Measure("out", new[] { "out/Report.txt" }, candidate => true));
		}

		[Fact]
		public void Not_finding_it_means_case_sensitive()
		{
			Assert.True(CaseProbe.Measure("out", new[] { "out/Report.txt" }, candidate => false));
		}

		/// <summary>
		/// Lazily, and only as far as it has to be: the first name with a letter in it
		/// settles the question, and a directory of a hundred thousand files costs one
		/// entry.
		/// </summary>
		[Fact]
		public void Stops_at_the_first_entry_that_can_answer()
		{
			var enumerated = new List<string>();

			CaseProbe.Measure("out", Watching(new[] { "out/2026", "out/Report.txt", "out/Other.txt" }, enumerated), candidate => false);

			Assert.Equal(new[] { "out/2026", "out/Report.txt" }, enumerated);
		}

		[Fact]
		public void A_directory_with_nothing_to_say_answers_nothing()
		{
			Assert.Null(CaseProbe.Measure("out", new string[0], candidate => true));
			Assert.Null(CaseProbe.Measure("out", new[] { "out/2026", "out/4.1" }, candidate => true));
		}

		private static IEnumerable<string> Watching(IEnumerable<string> entries, List<string> seen)
		{
			foreach (string entry in entries)
			{
				seen.Add(entry);
				yield return entry;
			}
		}

		[Fact]
		public void Probes_inside_the_directory_being_measured()
		{
			Assert.Equal("/mnt/ntfs/out/rEPORT.TXT", CaseProbe.FlippedSibling("/mnt/ntfs/out", "/mnt/ntfs/out/Report.txt"));
		}

		/// <summary>
		/// The defect this replaced: it flipped the directory's own name and looked for
		/// it in the parent, so an output under <c>/mnt/ntfs</c> measured <c>/mnt</c> —
		/// a different filesystem with a different answer.
		/// </summary>
		[Fact]
		public void Never_probes_the_parent()
		{
			string probe = CaseProbe.FlippedSibling("/mnt/ntfs", "/mnt/ntfs/Report.txt");

			Assert.StartsWith("/mnt/ntfs/", probe);
			Assert.NotEqual("/mnt/NTFS", probe);
		}

		[Fact]
		public void Accepts_a_bare_name_as_well_as_a_full_path()
		{
			Assert.Equal("out/rEPORT.TXT", CaseProbe.FlippedSibling("out", "Report.txt"));
		}

		[Fact]
		public void Flips_every_letter_rather_than_lowercasing()
		{
			Assert.Equal("out/rEPORT", CaseProbe.FlippedSibling("out", "Report"));
			Assert.Equal("out/report", CaseProbe.FlippedSibling("out", "REPORT"));
		}

		/// <summary>
		/// Written with escapes rather than the characters themselves so the test says
		/// the same thing whatever encoding reads this file.
		/// </summary>
		[Fact]
		public void Leaves_everything_that_is_not_an_ascii_letter_alone()
		{
			Assert.Equal("out/q4-2026_R\u00e9SUM\u00e9.pdf", CaseProbe.FlippedSibling("out", "Q4-2026_r\u00e9sum\u00e9.PDF"));
		}

		/// <summary>
		/// A name with no letter to flip is the same name, which would make the probe
		/// ask whether the entry it already has exists — always yes, and the directory
		/// would read as case-insensitive whatever it is. The caller moves on to the
		/// next entry instead.
		/// </summary>
		[Fact]
		public void Declines_a_name_with_no_letter_in_it()
		{
			Assert.Null(CaseProbe.FlippedSibling("out", "2026-01-31.4"));
			Assert.Null(CaseProbe.FlippedSibling("out", "_"));
		}

		[Fact]
		public void Reads_a_trailing_separator_as_part_of_neither_name()
		{
			Assert.Equal("out/sUB", CaseProbe.FlippedSibling("out/", "out/Sub/"));
		}

		[Fact]
		public void Reads_a_windows_separator_the_same_way()
		{
			Assert.Equal("C:/work/out/rEPORT.TXT", CaseProbe.FlippedSibling("C:/work/out", "C:\\work\\out\\Report.txt"));
		}
	}
}
