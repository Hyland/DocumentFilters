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

package com.hyland.documentfilters.cli.planning;

import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.discovery.InputSource;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.IFileSystem;
import com.hyland.documentfilters.cli.runtime.NoInputsError;
import com.hyland.documentfilters.cli.runtime.OutputConflictError;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Decides where every output goes, before anything is written.
 *
 * <p>The one rule underneath all of this: <b>the meaning of {@code -o} never depends
 * on {@code --to}</b>. Four cardinalities, chosen by the arguments alone:
 * <ul>
 * <li>{@link OutputCardinality#PER_INPUT} --- {@code --out-dir}, each input writes its own output</li>
 * <li>{@link OutputCardinality#COMBINED} --- {@code --combine --out-file book.pdf}</li>
 * <li>{@link OutputCardinality#SINGLE_TARGET} --- {@code --out-file} with exactly one input</li>
 * <li>{@link OutputCardinality#STREAM} --- {@code --stdout}, or {@code --out-file -}</li>
 * </ul>
 * These name target ownership only. How many <em>files</em> one input produces is
 * the separate question {@link FormatSpec#isSingleFile} answers: a .docx to PNG
 * writes one file per page and is still {@link OutputCardinality#PER_INPUT}.
 *
 * <p>{@code --out-file} with several inputs and no {@code --combine} is a hard error,
 * never a guess.
 */
public final class OutputPlanner
{
	private OutputPlanner()
	{
	}

	/**
	 * Builds the plan. Pure apart from asking {@code fileSystem} how it compares
	 * names, so it is exhaustively testable with a table of cases.
	 *
	 * @throws NoInputsError {@code inputs} is empty.
	 * @throws OutputConflictError {@code --out-file} with multiple inputs and no
	 *         {@code --combine}, or two inputs resolving to the same output path.
	 */
	public static RunPlan plan(SharedOptions shared, ConvertOptions convert, List<InputSource> inputs, IFileSystem fileSystem)
	{
		if (inputs.isEmpty())
			throw new NoInputsError("no inputs matched; the walk and filters selected nothing");

		RunPlan plan = new RunPlan();
		plan.format = convert.format;
		plan.cardinality = resolveCardinality(shared, convert, inputs.size());

		if (plan.cardinality == OutputCardinality.COMBINED)
			plan.combinedTarget = shared.output.outFile;

		FormatSpec format = convert.format;
		OutputNameTemplate template = shared.output.nameTemplate != null
			? shared.output.nameTemplate
			: OutputNameTemplate.defaultFor(format.isSingleFile());
		String outDir = shared.output.outDir != null ? shared.output.outDir : ".";
		PageRange pages = convert.pages != null ? convert.pages : PageRange.ALL;
		for (InputSource input : inputs)
		{
			PlannedItem item = new PlannedItem();
			item.sourcePath = input.path;
			item.displayName = input.path;
			item.pages = pages;

			switch (plan.cardinality)
			{
				case PER_INPUT:
					assignPerInputTarget(item, input, format, template, outDir);
					break;

				case SINGLE_TARGET:
					item.target = shared.output.outFile;
					break;

				default:
					// Neither COMBINED nor STREAM owns a path of its own: a combined
					// item feeds RunPlan.combinedTarget, and a streamed one writes to
					// stdout.
					break;
			}

			plan.items.add(item);
		}

		checkForCollisions(plan, fileSystem);
		checkOutputsAreNotInputs(plan, fileSystem);

		return plan;
	}

	/**
	 * Resolves one {@link OutputCardinality#PER_INPUT} item's target: the source tree
	 * replicated under {@code --out-dir}, then the naming template applied to the
	 * filename only.
	 */
	public static void assignPerInputTarget(PlannedItem item, InputSource input, FormatSpec format, OutputNameTemplate template,
		String outDir)
	{
		String relativeDirectory = directoryOf(input.relativePath != null ? input.relativePath : input.path);
		String directory = combineDirectories(outDir, relativeDirectory);

		if (format.isSingleFile())
			item.target = joinPath(directory, template.render(item.sourcePath, format.getExtension(), 1));
		else
			item.targetPattern = new TargetPattern(directory, template, format.getExtension());
	}

	/** The directory portion of a portable, forward-slash relative path. */
	public static String directoryOf(String relativePath)
	{
		String normalised = relativePath.replace('\\', '/');
		int slash = normalised.lastIndexOf('/');
		if (slash < 0)
			return "";
		return normalised.substring(0, slash);
	}

	private static String combineDirectories(String outDir, String relativeDirectory)
	{
		if (relativeDirectory == null || relativeDirectory.isEmpty())
			return outDir;

		if (outDir == null || outDir.isEmpty())
			return relativeDirectory;
		return outDir + "/" + relativeDirectory;
	}

	private static String joinPath(String directory, String fileName)
	{
		return directory == null || directory.isEmpty() ? fileName : directory + "/" + fileName;
	}

	/**
	 * The one part of planning that has to look at what is on disk: refusing to
	 * clobber existing files without {@code --overwrite}. Kept separate so that
	 * {@link #plan} itself stays pure.
	 *
	 * <p>For a single-file item this is the whole check. For a per-page item it
	 * checks only {@link TargetPattern#firstTarget}, because the remaining names
	 * depend on the document's page count; execution checks each page as it goes,
	 * through {@link #verifyPageTarget}.
	 *
	 * @throws OutputConflictError a target exists and {@code --overwrite} was not given.
	 */
	public static void verifyTargets(RunPlan plan, SharedOptions shared, IFileSystem fileSystem)
	{
		if (shared.output.overwrite)
			return;

		if (plan.cardinality == OutputCardinality.COMBINED)
		{
			verifyPageTarget(plan.combinedTarget, shared, fileSystem);
		}
		else
		{
			for (PlannedItem item : plan.items)
			{
				// A STREAM item owns neither: it writes to stdout, not a path.
				if (item.target == null && item.targetPattern == null)
					continue;

				verifyPageTarget(item.isPageCountDependent() ? item.targetPattern.firstTarget(item.sourcePath, item.pages) : item.target,
					shared, fileSystem);
			}
		}
	}

	/**
	 * The deferred half of {@link #verifyTargets}, called during execution once a
	 * page-count-dependent item's real filenames are known.
	 */
	public static void verifyPageTarget(String path, SharedOptions shared, IFileSystem fileSystem)
	{
		if (shared.output.overwrite)
			return;

		if (fileSystem.fileExists(path))
			throw new OutputConflictError(ErrorCodes.OUTPUT_EXISTS, path + " already exists; use --overwrite to replace it");
	}

	/** Chooses the cardinality from the output arguments and the input count. */
	public static OutputCardinality resolveCardinality(SharedOptions shared, ConvertOptions convert, int inputCount)
	{
		return resolveCardinality(shared, inputCount, convert.combine);
	}

	/**
	 * The same resolution, for a command with an output group but no
	 * {@code --combine} of its own --- extract, list and meta, which always pass
	 * {@code false}. Their output names come from the container once it is open,
	 * so they stop at cardinality rather than resolving a target.
	 */
	public static OutputCardinality resolveCardinality(SharedOptions shared, int inputCount, boolean combine)
	{
		if (shared.output.useStdout)
			return OutputCardinality.STREAM;

		if (shared.output.outFile != null)
		{
			if (combine)
				return OutputCardinality.COMBINED;

			if (inputCount > 1)
			{
				throw new OutputConflictError(ErrorCodes.OUT_FILE_MULTI_INPUT,
					"--out-file takes a single input; use --combine to merge several into one document");
			}

			return OutputCardinality.SINGLE_TARGET;
		}

		return OutputCardinality.PER_INPUT;
	}

	/**
	 * Detects two inputs resolving to the same output path, which happens when a
	 * flattened walk contains two files with the same stem.
	 *
	 * <p>For statically resolved items this compares the target paths directly. For
	 * page-count-dependent items it compares {@link TargetPattern#collisionKey},
	 * which catches the real-world case --- {@code a/report.docx} and
	 * {@code b/report.docx} both flattening into one output directory --- without
	 * needing to know how many pages either has.
	 */
	public static void checkForCollisions(RunPlan plan, IFileSystem fileSystem)
	{
		Set<String> seen = new HashSet<>();
		for (PlannedItem item : plan.items)
		{
			String key = item.target != null
				? item.target
				: item.targetPattern != null ? item.targetPattern.collisionKey(item.sourcePath) : null;
			if (key != null && !seen.add(collisionKeyFor(key, fileSystem)))
			{
				throw new OutputConflictError(ErrorCodes.OUTPUT_COLLISION,
					"two inputs would write to the same output; use --out-name to distinguish them");
			}
		}
	}

	/**
	 * Refuses a plan that would write over one of the documents it reads.
	 *
	 * <p>{@code --overwrite} says an existing file may be replaced, and an input is an
	 * existing file, so nothing else here stops {@code convert --to text -o . --overwrite
	 * notes.txt} from naming {@code notes.txt} as both. The combined case is the
	 * plainest: the output is created, and therefore truncated, before the first
	 * document is opened, so the source is gone and the run then fails to read it.
	 * A glob reaches the same place without a typo --- a second run over a directory
	 * picks up the first run's output as an input.
	 *
	 * <p>This is the collision check pointed at the other set: two outputs must not
	 * share a path, and an output must not share one with an input either. It uses
	 * the same key, so it folds case exactly where the filesystem does.
	 *
	 * <p>A page-count-dependent item is compared by its first target, for the reason
	 * {@link #verifyTargets} gives: the rest of the names are not known until the
	 * document is open. Page one is the one that would be written first, and
	 * writing it is what would destroy the source.
	 */
	public static void checkOutputsAreNotInputs(RunPlan plan, IFileSystem fileSystem)
	{
		Set<String> inputs = new HashSet<>();
		for (PlannedItem item : plan.items)
			inputs.add(collisionKeyFor(item.sourcePath, fileSystem));

		for (String target : plannedTargets(plan))
		{
			if (inputs.contains(collisionKeyFor(target, fileSystem)))
			{
				throw new OutputConflictError(ErrorCodes.OUTPUT_IS_INPUT,
					target + " is also an input; use --out-dir or --out-name to write somewhere else");
			}
		}
	}

	/** Every path the plan would write, as far as it is knowable before the run. */
	public static List<String> plannedTargets(RunPlan plan)
	{
		List<String> targets = new ArrayList<>();
		if (plan.combinedTarget != null)
			targets.add(plan.combinedTarget);

		for (PlannedItem item : plan.items)
		{
			if (item.target != null)
				targets.add(item.target);
			else if (item.targetPattern != null)
				targets.add(item.targetPattern.firstTarget(item.sourcePath, item.pages));
		}
		return targets;
	}

	/**
	 * A planned path reduced to what two outputs must not share.
	 *
	 * <p>The path itself where the filesystem tells names apart by case, and a
	 * case-folded copy where it does not. Comparing exactly everywhere would let
	 * {@code a/Report.docx} and {@code b/report.docx} both pass planning and then
	 * write to one file on Windows and macOS: the run would exit 0, report nothing,
	 * and the second output would silently replace the first.
	 *
	 * <p>Folding everywhere instead would be the other error --- on a case-sensitive
	 * filesystem those are two files, and refusing to write both declines work the
	 * tool can do correctly. So the question goes to {@link IFileSystem#isCaseSensitive}
	 * rather than to the host or to a fixed rule, which is also what keeps this
	 * testable: a fake filesystem states its own answer, so both behaviours are
	 * covered on every platform instead of only the one the suite happens to run on.
	 *
	 * <p>ASCII only, matching {@code ValueParsers.foldToken}'s reasoning rather than
	 * reusing it --- that one is for enumerated values and says paths are taken as
	 * typed. A locale-aware fold would make collision detection depend on the
	 * machine's locale; a Unicode-aware one would not be reproducible in all four
	 * languages without a shared table. Two names differing only outside ASCII
	 * therefore do not collide, in any of them.
	 *
	 * <p>What that costs, measured on NTFS: the two spellings of A-ring are one file
	 * there, so they flatten into one output directory, pass this check and then
	 * write to the same place. What stops the loss is no longer this key but the
	 * moment of the question: each target is looked at again immediately before it
	 * is written, when the earlier output is on disk and the filesystem --- the only
	 * authority on whether two names are one file --- answers for itself. The run
	 * then fails with {@code E_OUTPUT_EXISTS} rather than silently keeping the second.
	 *
	 * <p>Under {@code --overwrite} it still keeps the second, because that is what
	 * {@code --overwrite} asks for and nothing left can tell this apart from an
	 * ordinary replacement. A pair this key does fold is refused even then.
	 */
	public static String collisionKeyFor(String path, IFileSystem fileSystem)
	{
		if (fileSystem.isCaseSensitive(path))
			return path;

		char[] chars = path.toCharArray();
		for (int i = 0; i < chars.length; i++)
		{
			if (chars[i] >= 'A' && chars[i] <= 'Z')
				chars[i] = (char)(chars[i] - 'A' + 'a');
		}
		return new String(chars);
	}
}
