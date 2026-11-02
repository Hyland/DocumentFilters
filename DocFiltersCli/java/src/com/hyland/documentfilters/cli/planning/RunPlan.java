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

import java.util.ArrayList;
import java.util.List;

/**
 * The complete description of a run, produced before anything is opened or
 * written.
 *
 * <p>Holding the whole run as a value is what makes {@code --print-plan} possible,
 * and it is the only way to detect two inputs resolving to the same output path
 * before the first one has already been written.
 */
public final class RunPlan
{
	public OutputCardinality cardinality;

	/** One entry per input, in processing order. */
	public final List<PlannedItem> items = new ArrayList<>();

	/**
	 * The shared target when {@link #cardinality} is {@link OutputCardinality#COMBINED};
	 * otherwise null.
	 *
	 * <p>A run-level fact rather than a {@link PlannedItem}, because {@link #items}
	 * is keyed by <em>input</em>: one entry per document, in every cardinality and
	 * on every command. That is the unit execution works in, the unit
	 * {@code --on-error} counts, and the unit a combined run still needs --- a
	 * document that fails mid-combine is skipped while the rest are still written,
	 * so the run has to be able to fail one input and keep going.
	 *
	 * <p>Keying items by output instead would collapse a combined run into a single
	 * entry and lose that granularity.
	 */
	public String combinedTarget;

	/** The format every item renders to. */
	public FormatSpec format;

	/**
	 * Renders the plan in the canonical form the conformance corpus compares.
	 * All four language implementations must produce byte-identical output.
	 *
	 * <p>{@code cardinality} always appears; {@code target}/{@code pattern} lines do
	 * not, because extract, list and meta produce names that only exist once a
	 * document is open and so stop at cardinality. For {@link OutputCardinality#COMBINED}
	 * the one target is {@link #combinedTarget} rather than a per-item line, since
	 * every item feeds that single output. For {@link OutputCardinality#STREAM} no
	 * item owns a path at all.
	 */
	public String toCanonicalString()
	{
		List<String> lines = new ArrayList<>();
		lines.add("cardinality " + cardinalityText(cardinality));
		if (cardinality == OutputCardinality.COMBINED)
		{
			lines.add("target " + combinedTarget);
		}
		else
		{
			for (PlannedItem item : items)
			{
				if (item.target != null)
					lines.add("target " + item.target);
				else if (item.targetPattern != null)
					lines.add("pattern " + patternText(item.targetPattern));
			}
		}
		return String.join("\n", lines);
	}

	/** The canonical text for one cardinality, shared by every command that states one. */
	public static String cardinalityText(OutputCardinality cardinality)
	{
		switch (cardinality)
		{
			case PER_INPUT:
				return "per-input";
			case COMBINED:
				return "combined";
			case SINGLE_TARGET:
				return "single-target";
			case STREAM:
				return "stream";
			default:
				throw new IllegalArgumentException("cardinality");
		}
	}

	private static String patternText(TargetPattern pattern)
	{
		return pattern.directory == null || pattern.directory.isEmpty()
			? pattern.template.toString()
			: pattern.directory + "/" + pattern.template;
	}
}
