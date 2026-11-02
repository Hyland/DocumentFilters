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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_CLI_ARGUMENTTABLE_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_CLI_ARGUMENTTABLE_H

#include "ArgumentDefinition.h"
#include "Configuration/CommandOptions.h"
#include "Configuration/SharedOptions.h"

#include <memory>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Cli
			{
				/// <summary>
				/// Every argument the tool accepts, paired with what it does. The single
				/// source of truth for the command line surface.
				/// </summary>
				namespace ArgumentTable
				{
					/// <summary>
					/// The processing verbs, in help-display order. These are the commands
					/// that accept shared options, so this list drives <see cref="sharedFor"/>
					/// and the contract dump.
					/// </summary>
					const std::vector<std::string>& commands();

					/// <summary>
					/// Everything <c>docfilters &lt;verb&gt;</c> will dispatch, including the
					/// two that take no shared options at all: <c>help</c> and <c>version</c>
					/// are verbs, but not in <see cref="commands"/>, because they process no
					/// documents and accept no shared options.
					/// </summary>
					const std::vector<std::string>& allVerbs();

					/// <summary>The shared arguments a given verb accepts.</summary>
					/// <exception cref="std::invalid_argument"><paramref name="command"/> is not in <see cref="commands"/>.</exception>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> sharedFor(const std::string& command);

					/// <summary><c>-o/--out-dir</c>, <c>--out-file</c>, <c>--out-name</c>, <c>--stdout</c>, <c>--overwrite</c>.</summary>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> outputArguments();

					/// <summary><c>-s/--subfiles</c> and <c>--subfile-depth</c>.</summary>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> subfileArguments();

					/// <summary><c>-g/--glob</c> and <c>--regex</c>: selecting inputs by path.</summary>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> matchArguments();

					/// <summary><c>--include-types</c> and <c>--exclude-types</c>: selecting by detected format.</summary>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> typeArguments();

					/// <summary>The filesystem walk: <c>--files-from</c>, depth, <c>--follow</c>, <c>--sort</c>, <c>--include-hidden</c>.</summary>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> walkArguments();

					/// <summary><c>-p/--password</c>, <c>--password-file</c>, <c>--password-prompt</c>.</summary>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> passwordArguments();

					/// <summary><c>--options</c>, <c>--codepage</c>, <c>--temp-dir</c>.</summary>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> processingArguments();

					/// <summary><c>--on-error</c> and <c>--fail-fast</c>.</summary>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> batchArguments();

					/// <summary><c>--color</c>, <c>--verbose</c>, <c>--silent</c>. Every verb takes these.</summary>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> consoleArguments();

					/// <summary>Timing, limits, tracing and the <c>--debug-*</c> switches.</summary>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> diagnosticArguments();

					/// <summary><c>--license-key</c> and <c>--library</c>. Every verb that touches the engine takes these.</summary>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> libraryArguments();

					/// <summary>
					/// Arguments that work on every verb and are not part of any shared group:
					/// <c>--dry-run</c> (with its <c>--print-plan</c> alias) and <c>--print-contract</c>.
					/// </summary>
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> global();

					/// <summary>
					/// <c>convert</c>'s own arguments, including <c>--pages</c>.
					/// </summary>
					/// <remarks>
					/// The CLI design lists <c>--pages</c> among the shared options, but
					/// <c>convert</c> is the only command that renders pages and
					/// <c>ConvertOptions::pages</c> is its only destination. A shared
					/// definition is typed to <c>SharedOptions</c> and so could not reach that
					/// field at all, so it belongs here.
					/// </remarks>
					std::vector<ArgumentDefinition<Configuration::ConvertOptions>> convert();

					std::vector<ArgumentDefinition<Configuration::ExtractOptions>> extract();

					std::vector<ArgumentDefinition<Configuration::ListOptions>> list();

					std::vector<ArgumentDefinition<Configuration::IdentifyOptions>> identify();

					std::vector<ArgumentDefinition<Configuration::MetaOptions>> meta();

					std::vector<ArgumentDefinition<Configuration::FormatsOptions>> formats();

					std::vector<ArgumentDefinition<Configuration::OptionCatalogOptions>> optionCatalog();

					/// <summary>
					/// Every argument on a given verb --- command-specific plus shared --- as
					/// descriptors, for the contract dump and the coverage tests.
					/// </summary>
					/// <exception cref="std::invalid_argument"><paramref name="command"/> is not in <see cref="commands"/>.</exception>
					std::vector<std::shared_ptr<const IArgumentDescriptor>> describeCommand(const std::string& command);
				}
			}
		}
	}
}

#endif
