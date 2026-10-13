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

using System.Threading;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The per-document timing window, and <c>--timeout</c> on top of it.
	/// </summary>
	/// <remarks>
	/// Pinned in the order the commands actually call these: <c>BeginDocument</c>,
	/// work, <c>EndDocument</c>, then <c>CheckDocumentTimeout</c>. That order is the
	/// whole point — <c>EndDocument</c> used to discard the timer, so the check that
	/// ran immediately after always saw nothing and <c>--timeout</c> never fired for
	/// any document, however long it took.
	/// </remarks>
	public class DiagnosticsTests
	{
		/// <summary>A clock a test moves by hand, so a deadline can pass instantly.</summary>
		private sealed class TestClock
		{
			public TimeSpan Now { get; private set; }

			public TimeSpan Read() => Now;

			public void Advance(TimeSpan by) => Now += by;
		}

		private static readonly TestClock Clock = new TestClock();

		/// <remarks>
		/// Every case here drives the clock rather than sleeping. Asserting that a
		/// deadline passed used to mean waiting for it, which made the assertions both
		/// slow and approximate — a loaded machine could stretch a five-millisecond
		/// sleep past any limit, and a fast one could leave it short.
		/// </remarks>
		private static Diagnostics ForTimeout(int? seconds, IConsole console = null)
		{
			return new Diagnostics(new DiagnosticOptions { Timeout = seconds }, console ?? new RecordingConsole(),
								   engine: null, clock: Clock.Read);
		}

		[Fact]
		public void A_document_that_runs_past_the_timeout_is_reported_after_it_ends()
		{
			var diagnostics = ForTimeout(seconds: 30);

			diagnostics.BeginDocument("slow.pdf", 1, 1);
			Clock.Advance(TimeSpan.FromSeconds(31));
			diagnostics.EndDocument("slow.pdf");

			var error = Assert.Throws<LimitExceededError>(() => diagnostics.CheckDocumentTimeout("slow.pdf"));

			Assert.Equal(ErrorCodes.LimitExceeded, error.Code);
			Assert.Equal(ExitCode.LimitExceeded, error.ExitCode);
			Assert.Equal("slow.pdf", error.Path);
			Assert.Equal("--timeout of 30s exceeded", error.Message);
		}

		[Fact]
		public void A_document_inside_the_timeout_passes()
		{
			var diagnostics = ForTimeout(seconds: 600);

			diagnostics.BeginDocument("quick.pdf", 1, 1);
			diagnostics.EndDocument("quick.pdf");

			diagnostics.CheckDocumentTimeout("quick.pdf");
		}

		[Fact]
		public void Without_the_option_no_amount_of_time_is_a_failure()
		{
			var diagnostics = ForTimeout(seconds: null);

			diagnostics.BeginDocument("slow.pdf", 1, 1);
			Clock.Advance(TimeSpan.FromHours(1));
			diagnostics.EndDocument("slow.pdf");

			diagnostics.CheckDocumentTimeout("slow.pdf");
		}

		[Fact]
		public void The_check_is_harmless_before_any_document_has_begun()
		{
			ForTimeout(seconds: 0).CheckDocumentTimeout("nothing.pdf");
		}

		[Fact]
		public void Each_document_is_timed_from_its_own_start()
		{
			// The timer outlives EndDocument now, so the guard against it carrying over
			// is BeginDocument replacing it — not EndDocument clearing it.
			var diagnostics = ForTimeout(seconds: 600);

			diagnostics.BeginDocument("first.pdf", 1, 2);
			Clock.Advance(TimeSpan.FromSeconds(1));
			diagnostics.EndDocument("first.pdf");
			diagnostics.CheckDocumentTimeout("first.pdf");

			diagnostics.BeginDocument("second.pdf", 2, 2);
			diagnostics.EndDocument("second.pdf");
			diagnostics.CheckDocumentTimeout("second.pdf");
		}

		[Fact]
		public void Log_time_still_reports_the_documents_own_elapsed_time()
		{
			// EndDocument does two things; keeping the timer must not cost the other.
			var console = new RecordingConsole();
			var diagnostics = new Diagnostics(new DiagnosticOptions { LogTime = true }, console);

			diagnostics.BeginDocument("a.pdf", 1, 1);
			diagnostics.EndDocument("a.pdf");

			Assert.Contains(console.OutLines, line => line.StartsWith("a.pdf: ") && line.EndsWith("s"));
		}
	}
}
