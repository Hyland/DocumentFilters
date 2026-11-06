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

using System.Reflection;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Commands
{
	/// <summary>
	/// <c>version</c> — the tool version and the Document Filters build it is
	/// running against.
	/// </summary>
	/// <remarks>
	/// This is the one command that needs the engine without opening any document:
	/// the interesting half of the answer is the native library's version, which
	/// requires initialising it.
	/// </remarks>
	public sealed class VersionCommand : ICommand
	{
		public string Name => "version";

		public bool RequiresEngine => true;

		public int Execute(CommandContext context)
		{
			return CommandExecution.WithSession(context, () =>
			{
				// The informational version, not GetName().Version. An assembly
				// version is always four components, so <Version>1.0.0</Version>
				// comes back as "1.0.0.0" -- a shape no other implementation can
				// reproduce, because none of them has a fourth component to print.
				// The informational version is the <Version> string verbatim.
				//
				// Trimmed at '+' because a build with source-link metadata appends
				// one, as "1.0.0+<commit>", and that would differ between every
				// checkout rather than between every language.
				string toolVersion = Assembly.GetExecutingAssembly()
					.GetCustomAttribute<AssemblyInformationalVersionAttribute>()?
					.InformationalVersion.Split('+')[0] ?? "unknown";
				context.Console.WriteLine($"docfilters {toolVersion}");
				context.Console.WriteLine($"Document Filters {context.Session.DescribeVersion()}");
				return ExitCode.Success;
			});
		}

		/// <summary>
		/// <c>version</c> reports the tool and native library versions outright; a
		/// dry run has nothing further to plan.
		/// </summary>
		public string DescribePlan(CommandContext context)
		{
			return "command version";
		}
	}
}
