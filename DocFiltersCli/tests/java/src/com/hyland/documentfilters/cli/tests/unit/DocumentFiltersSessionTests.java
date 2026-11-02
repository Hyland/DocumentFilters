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

package com.hyland.documentfilters.cli.tests.unit;

import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;

import com.hyland.documentfilters.cli.configuration.LibraryOptions;
import com.hyland.documentfilters.cli.execution.DocumentFiltersSession;
import com.hyland.documentfilters.cli.tests.Test;

/**
 * Where {@code DocumentFiltersSession.create} looks for the engine's own
 * resources --- fonts included.
 *
 * <p>doc2text and the other published samples default this to {@code "."} and get
 * away with it because they always run from their own build output directory.
 * This tool is invoked from wherever the user's documents happen to be, so
 * {@code "."} there resolves to a directory with no fonts at all.
 * {@code resolveLibraryPath} is the precedence decision alone, pure and without
 * the environment or the real jar location, so it is testable without either.
 *
 * <p>In this port the path chooses the engine's resource directory only: which
 * native library is loaded is fixed by {@code java.library.path} before the
 * program starts.
 */
public final class DocumentFiltersSessionTests
{
	@Test
	public void anExplicitLibraryPathWinsOverEverything()
	{
		LibraryOptions options = new LibraryOptions();
		options.libraryPath = "/opt/df";

		String resolved = DocumentFiltersSession.resolveLibraryPath(options, "/env/df", "/exe/dir");

		assertEquals("/opt/df", resolved);
	}

	@Test
	public void theEnvironmentVariableWinsOverTheExecutableDirectory()
	{
		String resolved = DocumentFiltersSession.resolveLibraryPath(new LibraryOptions(), "/env/df", "/exe/dir");

		assertEquals("/env/df", resolved);
	}

	@Test
	public void withNeitherArgumentNorEnvironmentVariableItFallsBackToTheExecutableDirectory()
	{
		// Not the process's current directory: a general-purpose CLI is invoked from
		// wherever the user's documents are, not from its own install directory, and
		// "." would send font lookup there instead.
		String resolved = DocumentFiltersSession.resolveLibraryPath(new LibraryOptions(), null, "/exe/dir");

		assertEquals("/exe/dir", resolved);
	}
}
