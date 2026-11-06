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

package com.hyland.documentfilters.cli.configuration;

import java.util.List;

/**
 * Everything the shared options resolved to. One instance per run, filled in by
 * the argument table's setters, then checked by {@link OptionsValidator}.
 *
 * <p><b>Two arguments never share one field.</b> Where two arguments describe the same
 * setting, each records its own presence and the setting is derived from both ---
 * {@code --max-depth} plus {@code noRecurseWasGiven} gives
 * {@link InputOptions#effectiveMaxDepth}, {@code --log-level} plus
 * {@code --debug-log-level} gives {@link DiagnosticOptions#effectiveLogLevel}.
 * Collapsing a pair into one field makes a contradiction indistinguishable from a
 * plain restatement, so it could never be reported --- and reporting it is the point,
 * because argv order is not portable enough to resolve such a pair by precedence.
 *
 * <p>Defaults live here as field initialisers, never in the parser declarations ---
 * four declarations of the same default drift, one does not.
 */
public final class SharedOptions
{
	public final InputOptions input = new InputOptions();

	public final OutputOptions output = new OutputOptions();

	public final SubfileOptions subfiles = new SubfileOptions();

	public final PasswordOptions passwords = new PasswordOptions();

	public final ProcessingOptions processing = new ProcessingOptions();

	public final BatchOptions batch = new BatchOptions();

	public final ConsoleOptions console = new ConsoleOptions();

	/**
	 * {@code --dry-run}, or its alias {@code --print-plan}: run stages 1 to 3, print
	 * the resulting plan, and stop without opening a document.
	 *
	 * <p>Two arguments write this one field, which is safe here where it would not be
	 * elsewhere: both set it to true, so giving both is redundant rather than
	 * contradictory and there is nothing to reconcile. Aliases that carry a
	 * <em>value</em> --- {@code --log-level} and {@code --debug-log-level} --- get a
	 * field each instead.
	 */
	public boolean dryRun;

	/**
	 * {@code --print-contract}: emit the whole argument surface as flat text and
	 * stop. A debugging aid, and the same information {@code help --json} returns.
	 */
	public boolean printContract;

	public final DiagnosticOptions diagnostics = new DiagnosticOptions();

	public final LibraryOptions library = new LibraryOptions();

	/** {@link #toCanonicalString(boolean)} with the {@code subfiles} line. */
	public String toCanonicalString()
	{
		return toCanonicalString(true);
	}

	/**
	 * Renders the options in the canonical form used by the conformance corpus:
	 * the {@code walk}, {@code match}, {@code types}, {@code subfiles} and
	 * {@code on-error} lines of a {@code --print-plan} dump, in that order, one per
	 * line, with no trailing newline.
	 *
	 * <p>That order is contiguous exactly as written, for every command that has
	 * these fields: a command's own fields --- {@code convert}'s {@code pages};
	 * {@code extract}'s {@code include}/{@code exclude}/{@code images}/
	 * {@code make-dirs}/{@code sub-id}; {@code list}'s and {@code meta}'s
	 * {@code json}; every command's {@code cardinality} --- always come after this
	 * whole block, never inside it, so a caller appends this string and then
	 * its own lines.
	 *
	 * <p>All four language implementations must produce byte-identical output for
	 * each line.
	 *
	 * @param includeSubfiles whether to include the {@code subfiles} line.
	 */
	public String toCanonicalString(boolean includeSubfiles)
	{
		StringBuilder sb = new StringBuilder();

		Integer depth = input.effectiveMaxDepth();
		sb.append("walk depth=").append(depth == null ? "unlimited" : depth.toString())
			.append(" follow=").append(input.followSymlinks ? "yes" : "no")
			.append(" sort=").append(input.sort ? "yes" : "no")
			.append(" hidden=").append(input.includeHidden ? "yes" : "no")
			.append('\n');

		sb.append("match ").append(input.matcher == null ? "none" : input.matcher.toString()).append('\n');

		sb.append("types include=").append(formatIdList(input.includeTypes))
			.append(" exclude=").append(formatIdList(input.excludeTypes))
			.append('\n');

		if (includeSubfiles)
		{
			sb.append("subfiles ");
			if (subfiles.enabled)
				sb.append("enabled depth=").append(subfiles.maxDepth == null ? "unlimited" : subfiles.maxDepth.toString());
			else
				sb.append("disabled");
			sb.append('\n');
		}

		sb.append("on-error ").append(batch.onError == OnErrorBehaviour.STOP ? "stop" : "skip");

		return sb.toString();
	}

	private static String formatIdList(List<String> ids)
	{
		return ids.isEmpty() ? "none" : String.join(",", ids);
	}
}
