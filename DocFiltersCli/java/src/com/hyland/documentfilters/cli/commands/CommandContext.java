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

import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.execution.DocumentFiltersSession;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.IFileSystem;
import com.hyland.documentfilters.cli.runtime.Interruption;
import java.time.Duration;
import java.util.function.BiConsumer;
import java.util.function.Supplier;

/**
 * Everything a command needs that is not its own options. Per-command options
 * are passed to the command's constructor instead, so there is no object here
 * holding seven verbs' worth of options with six of them unused.
 */
public final class CommandContext
{
	public SharedOptions sharedOptions;

	public IConsole console;

	public IFileSystem fileSystem;

	/**
	 * The initialised engine. Null for commands that do not open documents, and
	 * created lazily so that {@code --print-plan} never needs a license.
	 */
	public DocumentFiltersSession session;

	/**
	 * Whether the user has interrupted the run, or null when nothing is watching.
	 *
	 * <p>Null for a caller that installed no handler --- every test, and
	 * {@code --dry-run}, which finishes long before anyone could press anything.
	 */
	public Interruption interruption;

	/**
	 * A monotonic reading for {@code Diagnostics}, or null for the real one.
	 *
	 * <p>Here rather than inside each command because the commands build their own
	 * {@code Diagnostics} and a test driving the whole CLI has no other way in.
	 */
	public Supplier<Duration> clock;

	/**
	 * What {@code --view} hands the finished output to, or null for the desktop's
	 * own handler.
	 *
	 * <p>Here for the same reason {@link #clock} is: a test driving the whole CLI
	 * has no other way in, and what {@code --view} has to get right is
	 * <em>when</em> --- the output has to be finished before anything is asked to
	 * open it.
	 */
	public BiConsumer<String, IConsole> openViewer;
}
