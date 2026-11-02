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

package com.hyland.documentfilters.cli.runtime;

import java.awt.Desktop;
import java.io.File;
import java.io.IOException;
import java.util.Locale;

/**
 * {@code --view}: opens a finished output in whatever the desktop uses for it.
 *
 * <p>Best effort. A failure is reported and the run carries on, because nothing
 * about the document failed and the output is on disk either way.
 */
public final class Viewer
{
	private Viewer()
	{
	}

	public static void open(String path, IConsole console)
	{
		if (path == null || path.isEmpty())
			return;

		try
		{
			start(path);
		}
		catch (IOException | RuntimeException | LinkageError error)
		{
			console.writeError("--view could not open " + path + ": " + error.getMessage());
		}
	}

	private static void start(String path) throws IOException
	{
		String os = System.getProperty("os.name", "").toLowerCase(Locale.ROOT);
		if (os.startsWith("windows"))
		{
			// The only case that needs the shell: Desktop.open is ShellExecute, which
			// is what makes the file association apply. It takes the path as the
			// verb's target rather than as a command line, so there is nothing to
			// quote --- unlike "cmd /c start", which would re-parse an '&' in a name.
			Desktop.getDesktop().open(new File(path));
			return;
		}

		String launcher = os.startsWith("mac") ? "open" : "xdg-open";
		new ProcessBuilder(launcher, path).inheritIO().start();
	}
}
