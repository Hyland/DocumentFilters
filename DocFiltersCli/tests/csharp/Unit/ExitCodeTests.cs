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

using Hyland.DocumentFilters.Cli.Runtime;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The exit code table, which every implementation shares.
	/// </summary>
	/// <remarks>
	/// <c>isys_doc2text</c> returns only 0 or 1, and returns 0 even on a usage error,
	/// so there was no meaningful parity to preserve beyond "non-zero means trouble".
	/// These values are the agreed replacement.
	/// </remarks>
	public class ExitCodeTests
	{
		[Fact]
		public void The_agreed_values_are_stable()
		{
			Assert.Equal(0, ExitCode.Success);
			Assert.Equal(1, ExitCode.PartialFailure);
			Assert.Equal(2, ExitCode.UsageError);
			Assert.Equal(3, ExitCode.NoInputsMatched);
			Assert.Equal(4, ExitCode.OutputConflict);
			Assert.Equal(5, ExitCode.InitialisationFailure);
			Assert.Equal(6, ExitCode.LimitExceeded);
			Assert.Equal(130, ExitCode.Interrupted);
		}

		[Fact]
		public void A_usage_error_exits_two()
		{
			var error = new UsageError(ErrorCodes.UnknownArgument, "unknown option --nonsense");

			Assert.Equal(ExitCode.UsageError, error.ExitCode);
		}

		[Fact]
		public void No_inputs_exits_three()
		{
			Assert.Equal(ExitCode.NoInputsMatched, new NoInputsError("nothing matched").ExitCode);
		}

		[Fact]
		public void An_output_conflict_exits_four()
		{
			var error = new OutputConflictError(ErrorCodes.OutFileMultiInput, "several inputs, one --out-file");

			Assert.Equal(ExitCode.OutputConflict, error.ExitCode);
		}

		[Fact]
		public void An_initialisation_failure_exits_five()
		{
			Assert.Equal(ExitCode.InitialisationFailure, new InitialisationError("no license").ExitCode);
		}

		[Fact]
		public void A_limit_being_exceeded_exits_six()
		{
			Assert.Equal(ExitCode.LimitExceeded, new LimitExceededError("--max-time passed").ExitCode);
		}

		[Fact]
		public void A_single_document_failure_maps_to_partial_failure()
		{
			var error = new DocumentError("a.docx", "could not open");

			Assert.Equal(ExitCode.PartialFailure, error.ExitCode);
		}

		[Fact]
		public void Every_error_carries_a_machine_readable_code()
		{
			// The corpus matches on the code, not the prose, so the message can be
			// improved without rewriting every fixture.
			Assert.Equal(ErrorCodes.UnknownArgument, new UsageError(ErrorCodes.UnknownArgument, "x").Code);
			Assert.Equal(ErrorCodes.NoInputs, new NoInputsError("x").Code);
			Assert.Equal(ErrorCodes.DocumentFailed, new DocumentError("a", "x").Code);
		}

		[Fact]
		public void A_document_error_remembers_which_file_failed()
		{
			var error = new DocumentError("corpus/a.docx", "could not open");

			Assert.Equal("corpus/a.docx", error.Path);
		}
	}
}
