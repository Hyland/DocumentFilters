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

import com.hyland.documentfilters.cli.runtime.ExitCode;

/**
 * {@code version} --- the tool version and the Document Filters build it is
 * running against.
 *
 * <p>This is the one command that needs the engine without opening any document:
 * the interesting half of the answer is the native library's version, which
 * requires initialising it.
 */
public final class VersionCommand implements ICommand
{
	@Override
	public String getName()
	{
		return "version";
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
			context.console.writeLine("docfilters " + toolVersion());
			context.console.writeLine("Document Filters " + context.session.describeVersion());
			return ExitCode.SUCCESS;
		});
	}

	/**
	 * {@code version} reports the tool and native library versions outright; a
	 * dry run has nothing further to plan.
	 */
	@Override
	public String describePlan(CommandContext context)
	{
		return "command version";
	}

	/**
	 * This tool's own version, from the jar manifest's
	 * {@code Implementation-Version}, or "unknown" when run from loose classes that
	 * have no manifest --- the same fallback the reference gives for an assembly
	 * with no version.
	 */
	private static String toolVersion()
	{
		Package source = VersionCommand.class.getPackage();
		String version = source != null ? source.getImplementationVersion() : null;
		return version != null ? version : "unknown";
	}
}
