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

package com.hyland.documentfilters.cli.cli;

import com.hyland.documentfilters.cli.commands.ICommand;
import com.hyland.documentfilters.cli.configuration.SharedOptions;

/**
 * The result of stages 1 and 2: a command ready to run, and the shared options it
 * will run under.
 *
 * <p>Both halves are returned because both are needed afterwards and neither
 * contains the other. The command holds its own per-verb options privately; the
 * options hold everything shared. Returning them together also means the binding
 * can be tested directly, rather than only through whatever the command happens
 * to print.
 */
public final class BoundCommandLine
{
	private final ICommand _command;
	private final SharedOptions _sharedOptions;

	public BoundCommandLine(ICommand command, SharedOptions sharedOptions)
	{
		_command = command;
		_sharedOptions = sharedOptions;
	}

	/** The selected verb, with its own options already bound. */
	public ICommand getCommand()
	{
		return _command;
	}

	/** The shared options, already bound. */
	public SharedOptions getSharedOptions()
	{
		return _sharedOptions;
	}
}
