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

package com.hyland.documentfilters.cli.commands;

import com.hyland.documentfilters.cli.configuration.OptionCatalogOptions;
import com.hyland.documentfilters.cli.execution.CatalogReports;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import java.util.Arrays;

/**
 * {@code options} --- list the engine processing options, the tunables passed via
 * {@code --options}, with type, default and scope. Takes no input files.
 */
public final class OptionsCommand implements ICommand
{
	/** The bound per-command options. Readable so the binding itself can be tested. */
	public final OptionCatalogOptions options;

	public OptionsCommand(OptionCatalogOptions options)
	{
		this.options = options;
	}

	@Override
	public String getName()
	{
		return "options";
	}

	@Override
	public boolean requiresEngine()
	{
		return true;
	}

	@Override
	public int execute(CommandContext context)
	{
		return CommandExecution.withSession(context, () ->
		{
			new CatalogReports(context.session, context.console).writeOptions(options);
			return ExitCode.SUCCESS;
		});
	}

	@Override
	public String describePlan(CommandContext context)
	{
		return String.join("\n", Arrays.asList(
			"command options",
			"json " + PlanFormat.yesNo(options.json),
			"filter " + PlanFormat.noneOr(options.filter),
			"applies-to " + PlanFormat.noneOr(options.appliesTo)));
	}
}
