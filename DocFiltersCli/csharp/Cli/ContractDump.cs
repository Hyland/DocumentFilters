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

using System.Text;
using System.Text.Json;

namespace Hyland.DocumentFilters.Cli.Cli
{
	/// <summary>
	/// Emits the complete command line surface as flat text, for
	/// <c>--print-contract</c>.
	/// </summary>
	/// <remarks>
	/// A debugging aid, not the contract: the authority on which arguments exist is
	/// <c>tests/conformance/arguments.txt</c>, and this dump is checked against it.
	///
	/// <see cref="RenderJson"/> answers <c>help --json</c> with the same information.
	/// The two are separate renderers over one argument table: this output is
	/// brace-free, so an argument named <c>--json</c> cannot share it.
	///
	/// The flat format is line-oriented and free of JSON, so all four languages can
	/// parse it with nothing but their standard library. Each argument line's four
	/// columns — the literal word <c>argument</c>, the template, the arity, and the
	/// description — are tab-separated, so a reader splits once on tab and never
	/// has to guess where the free-form description begins. Splitting on space
	/// instead would find the words "and", "or" and stray flag mentions inside a
	/// help string exactly as readily as an argument name, since English prose and
	/// a template are otherwise indistinguishable to a naive whitespace split.
	///
	/// <code>
	/// command convert
	/// argument\t--to\tsingle\tTarget format
	/// argument\t-o|--out-dir\tsingle\tWrite into a directory
	/// argument\t--overwrite\tnone\tReplace existing outputs
	/// </code>
	/// </remarks>
	public static class ContractDump
	{
		/// <summary>Renders the whole surface: every command, and every argument on it.</summary>
		public static string Render()
		{
			var sb = new StringBuilder();
			foreach (string command in ArgumentTable.Commands)
				sb.Append(RenderCommand(command));
			return sb.ToString();
		}

		/// <summary>Renders one command's section.</summary>
		public static string RenderCommand(string command)
		{
			var sb = new StringBuilder();
			sb.Append("command ").Append(command).Append('\n');
			foreach (IArgumentDescriptor argument in ArgumentTable.DescribeCommand(command))
			{
				sb.Append("argument\t")
					.Append(argument.Template).Append('\t')
					.Append(ArityText(argument.Arity)).Append('\t')
					.Append(argument.Description)
					.Append('\n');
			}
			return sb.ToString();
		}

		/// <summary>
		/// The same surface as JSON, for <c>help --json</c>.
		/// </summary>
		/// <remarks>
		/// A separate renderer rather than a reformatting of <see cref="Render"/>:
		/// that output is deliberately line-oriented and free of braces, so an
		/// argument named <c>--json</c> could not honestly share it. Both read the same
		/// argument table, which is what keeps them in agreement.
		/// </remarks>
		public static string RenderJson()
		{
			var commands = ArgumentTable.Commands.Select(command => new
			{
				command,
				arguments = ArgumentTable.DescribeCommand(command).Select(argument => new
				{
					template = argument.Template,
					description = argument.Description,
					arity = ArityText(argument.Arity)
				})
			});
			return JsonSerializer.Serialize(commands) + "\n";
		}

		private static string ArityText(ArgumentArity arity)
		{
			switch (arity)
			{
				case ArgumentArity.NoValue:
					return "none";
				case ArgumentArity.SingleValue:
					return "single";
				case ArgumentArity.MultipleValue:
					return "multiple";
				default:
					throw new ArgumentOutOfRangeException(nameof(arity));
			}
		}
	}
}
