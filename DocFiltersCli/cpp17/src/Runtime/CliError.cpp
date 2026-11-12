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

#include "CliError.h"

#include "ExitCode.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				namespace ErrorCodes
				{
					const std::vector<std::string>& all()
					{
						static const std::vector<std::string> codes = {
							unknownCommand, unknownArgument, missingValue, missingArgument, badValue, badPageRange,
							badTemplate, unknownFormat, mutuallyExclusive, combineUnsupported,
							stdoutNeedsSingleFile, outFileNeedsSingleFile, outNameNeedsOutDir, templateNeedsPage, reportNeedsHiDef,
							subfileDepthNeedsSubfiles,
							outFileMultiInput, outputExists, outputCollision, unsafeOutput, outputIsInput, noInputs, initFailed,
							limitExceeded, documentFailed, interrupted
						};
						return codes;
					}
				}

				int UsageError::exitCode() const { return ExitCode::usageError; }

				int NoInputsError::exitCode() const { return ExitCode::noInputsMatched; }

				int OutputConflictError::exitCode() const { return ExitCode::outputConflict; }

				int InitialisationError::exitCode() const { return ExitCode::initialisationFailure; }

				int LimitExceededError::exitCode() const { return ExitCode::limitExceeded; }

				int DocumentError::exitCode() const { return ExitCode::partialFailure; }
			}
		}
	}
}
