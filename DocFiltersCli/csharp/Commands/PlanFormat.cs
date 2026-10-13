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

using Hyland.DocumentFilters.Cli.Discovery;

namespace Hyland.DocumentFilters.Cli.Commands
{
	/// <summary>
	/// The handful of rendering rules every command's <see cref="ICommand.DescribePlan"/>
	/// shares, kept in one place so the conformance corpus's vocabulary — "yes"/"no",
	/// "none" for an absent value — is spelled the same way everywhere.
	/// </summary>
	internal static class PlanFormat
	{
		public static string YesNo(bool value) => value ? "yes" : "no";

		public static string NoneOr(string value) => string.IsNullOrEmpty(value) ? "none" : value;

		public static string NoneOr(IReadOnlyList<string> values) => values.Count == 0 ? "none" : string.Join(", ", values);

		public static string Inputs(IReadOnlyList<InputSource> inputs) => "inputs " + string.Join(", ", inputs.Select(i => i.Path));
	}
}
