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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_CONFIGURATION_OPTIONSVALIDATOR_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_CONFIGURATION_OPTIONSVALIDATOR_H

#include "CommandOptions.h"
#include "SharedOptions.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Configuration
			{
				/// <summary>
				/// The rules that no single argument can enforce on its own, because they
				/// are relations between two or more arguments.
				/// </summary>
				/// <remarks>
				/// Everything here is decidable from the command line alone. Rules that
				/// need to know how many input files actually exist --- <c>--out-file</c>
				/// with more than one input, for instance --- belong in
				/// <c>Planning::OutputPlanner</c>, which runs after the filesystem walk.
				///
				/// The individual per-rule checks are implementation details, not part of
				/// this header: C#'s reference keeps them <c>internal</c>, reachable from
				/// its own test assembly but not part of the public surface, and nothing
				/// in that test suite calls one directly rather than through
				/// <see cref="validate"/>, <see cref="validateConvert"/> or
				/// <see cref="validateExtract"/> --- so they stay private to the .cpp here
				/// too.
				/// </remarks>
				namespace OptionsValidator
				{
					/// <summary>Runs every shared-option rule.</summary>
					/// <exception cref="Runtime::UsageError">A rule was broken.</exception>
					void validate(const SharedOptions& shared);

					/// <summary>Runs the <c>convert</c>-specific rules in addition to the shared ones.</summary>
					void validateConvert(const SharedOptions& shared, const ConvertOptions& options);

					/// <summary>Runs the <c>extract</c>-specific rules in addition to the shared ones.</summary>
					void validateExtract(const SharedOptions& shared, const ExtractOptions& options);
				}
			}
		}
	}
}

#endif
