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

import com.hyland.documentfilters.cli.cli.ContractDump;
import com.hyland.documentfilters.cli.runtime.ExitCode;

/**
 * {@code help [command]} --- usage for the tool, or for one command.
 *
 * <p>The plain-text rendering is deliberately outside the cross-language
 * contract. {@code help --json} is not: it emits the command and argument
 * reference as structured data, so it must be identical everywhere, and it
 * renders through {@link ContractDump#renderJson} rather than the line-oriented
 * {@link ContractDump#render}. The two share the argument table, not the renderer.
 */
public final class HelpCommand implements ICommand
{
	private final String _topic;
	private final boolean _json;

	public HelpCommand(String topic, boolean json)
	{
		_topic = topic;
		_json = json;
	}

	@Override
	public String getName()
	{
		return "help";
	}

	@Override
	public boolean requiresEngine()
	{
		return false;
	}

	@Override
	public int execute(CommandContext context)
	{
		if (_json)
			context.console.writeLine(ContractDump.renderJson());
		else
			context.console.writeLine(_topic == null ? ContractDump.render() : ContractDump.renderCommand(_topic));

		return ExitCode.SUCCESS;
	}

	/**
	 * {@code help} processes no documents, so {@code --dry-run} has nothing to add
	 * to what running it outright already does: it never opens the engine.
	 */
	@Override
	public String describePlan(CommandContext context)
	{
		return "command help";
	}
}
