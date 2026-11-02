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

package com.hyland.documentfilters.cli.execution;

import com.hyland.documentfilters.cli.configuration.PasswordOptions;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.IFileSystem;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Supplies passwords to the engine as it encounters protected documents.
 *
 * <p>Candidates come from {@code -p}, then {@code --password-file}, in order. When
 * none of them work and {@code --password-prompt} was given, the user is asked.
 *
 * <p><b>Open, and deliberately so:</b> what {@code --password-prompt} should do
 * when stdin is not a terminal --- fail with a usage error, or fall through as if
 * it were not given. The design document carries it as one of the calls to be
 * made while building, so the four ports must agree on an answer before any of
 * them relies on one.
 */
public final class PasswordProvider
{
	private final PasswordOptions _options;
	private final IConsole _console;
	private final IFileSystem _fileSystem;

	private List<String> _fileCandidates;
	private int _nextIndex;
	private boolean _promptExhausted;

	public PasswordProvider(PasswordOptions options, IConsole console, IFileSystem fileSystem)
	{
		_options = options;
		_console = console;
		_fileSystem = fileSystem;
	}

	/**
	 * Returns the next password to try for the given document, or null when the
	 * candidates are exhausted.
	 *
	 * <p>{@code -p} candidates are tried in the order given, then every line of
	 * {@code --password-file}, and finally --- once those are exhausted and
	 * {@code --password-prompt} was given --- one interactive prompt. The prompt is
	 * asked at most once per document.
	 */
	public String getPassword(String documentId)
	{
		if (_fileCandidates == null)
			loadPasswordFile();

		List<String> candidates = _options.passwords;
		if (_nextIndex < candidates.size())
			return candidates.get(_nextIndex++);

		int fileIndex = _nextIndex - candidates.size();
		if (fileIndex < _fileCandidates.size())
		{
			_nextIndex++;
			return _fileCandidates.get(fileIndex);
		}

		if (_options.prompt && !_promptExhausted && _console.isInputInteractive())
		{
			_promptExhausted = true;
			return _console.readPassword("Password for " + documentId + ": ");
		}

		return null;
	}

	/** Forgets which candidates have been tried, when moving to a new document. */
	public void reset()
	{
		_nextIndex = 0;
		_promptExhausted = false;
	}

	/**
	 * Reads {@code --password-file}, one candidate per line.
	 *
	 * <p>A file that cannot be read is the platform's failure, not a
	 * {@code CliError}, as in the reference.
	 */
	public void loadPasswordFile()
	{
		List<String> candidates = new ArrayList<>();
		if (_options.passwordFile != null && !_options.passwordFile.isEmpty())
		{
			List<String> lines;
			try
			{
				lines = _fileSystem.readAllLines(_options.passwordFile);
			}
			catch (IOException e)
			{
				throw new UncheckedIOException(e.getMessage(), e);
			}
			for (String line : lines)
			{
				if (!line.isEmpty())
					candidates.add(line);
			}
		}
		_fileCandidates = candidates;
	}
}
