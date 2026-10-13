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

using Hyland.DocumentFilters.Cli.Cli;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Commands
{
	/// <summary>
	/// <c>help [command]</c> — usage for the tool, or for one command.
	/// </summary>
	/// <remarks>
	/// The plain-text rendering comes from the CLI parsing library and therefore
	/// differs slightly between the four language implementations. That is accepted:
	/// help text is deliberately outside the cross-language contract.
	///
	/// <c>help --json</c> is different. It emits the command and argument reference as
	/// structured data, so it must be identical everywhere, and it renders through
	/// <c>ContractDump.RenderJson</c> rather than the line-oriented
	/// <c>ContractDump.Render</c>. The two share the argument table, not the
	/// renderer.
	/// </remarks>
	public sealed class HelpCommand : ICommand
	{
		private readonly string _topic;
		private readonly bool _json;

		public HelpCommand(string topic, bool json)
		{
			_topic = topic;
			_json = json;
		}

		public string Name => "help";

		public bool RequiresEngine => false;

		public int Execute(CommandContext context)
		{
			if (_json)
				context.Console.WriteLine(ContractDump.RenderJson());
			else
				context.Console.WriteLine(_topic == null ? ContractDump.Render() : ContractDump.RenderCommand(_topic));

			return ExitCode.Success;
		}

		/// <summary>
		/// <c>help</c> processes no documents, so <c>--dry-run</c> has nothing to add
		/// to what running it outright already does: it never opens the engine.
		/// </summary>
		public string DescribePlan(CommandContext context)
		{
			return "command help";
		}
	}
}
