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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_CLI_CONTRACTDUMP_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_CLI_CONTRACTDUMP_H

#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Cli
			{
				/// <summary>
				/// Emits the complete command line surface as flat text, for
				/// <c>--print-contract</c>.
				/// </summary>
				/// <remarks>
				/// A debugging aid, not the contract: the authority on which arguments exist
				/// is <c>tests/conformance/arguments.txt</c>, and this dump is checked against
				/// it.
				///
				/// <see cref="renderJson"/> answers <c>help --json</c> with the same
				/// information. The two are separate renderers over one argument table: this
				/// output is brace-free, so an argument named <c>--json</c> cannot share it.
				///
				/// The flat format is line-oriented and free of JSON, so all four languages
				/// can parse it with nothing but their standard library. Each argument line's
				/// four columns --- the literal word <c>argument</c>, the template, the arity,
				/// and the description --- are tab-separated, so a reader splits once on tab
				/// and never has to guess where the free-form description begins.
				///
				/// <code>
				/// command convert
				/// argument\t--to\tsingle\tTarget format
				/// argument\t-o|--out-dir\tsingle\tWrite into a directory
				/// argument\t--overwrite\tnone\tReplace existing outputs
				/// </code>
				/// </remarks>
				namespace ContractDump
				{
					/// <summary>Renders the whole surface: every command, and every argument on it.</summary>
					std::string render();

					/// <summary>Renders one command's section.</summary>
					std::string renderCommand(const std::string& command);

					/// <summary>
					/// The same surface as JSON, for <c>help --json</c>.
					/// </summary>
					/// <remarks>
					/// A separate renderer rather than a reformatting of <see cref="render"/>:
					/// that output is deliberately line-oriented and free of braces, so an
					/// argument named <c>--json</c> could not honestly share it. Both read the
					/// same argument table, which is what keeps them in agreement.
					/// </remarks>
					std::string renderJson();
				}
			}
		}
	}
}

#endif
