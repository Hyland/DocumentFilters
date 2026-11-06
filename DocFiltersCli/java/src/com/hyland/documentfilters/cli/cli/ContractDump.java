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

import com.hyland.documentfilters.cli.runtime.JsonWriter;

/**
 * Emits the complete command line surface as flat text, for {@code --print-contract}.
 *
 * <p>A debugging aid, not the contract: the authority on which arguments exist is
 * {@code tests/conformance/arguments.txt}, and this dump is checked against it.
 *
 * <p>{@link #renderJson} answers {@code help --json} with the same information. The
 * two are separate renderers over one argument table: this output is brace-free, so
 * an argument named {@code --json} cannot share it.
 *
 * <p>The flat format is line-oriented and free of JSON, so all four languages can
 * parse it with nothing but their standard library. Each argument line's four
 * columns --- the literal word {@code argument}, the template, the arity, and the
 * description --- are tab-separated, so a reader splits once on tab and never has
 * to guess where the free-form description begins.
 *
 * <pre>
 * command convert
 * argument\t--to\tsingle\tTarget format
 * argument\t-o|--out-dir\tsingle\tWrite into a directory
 * argument\t--overwrite\tnone\tReplace existing outputs
 * </pre>
 */
public final class ContractDump
{
	private ContractDump()
	{
	}

	/** Renders the whole surface: every command, and every argument on it. */
	public static String render()
	{
		StringBuilder sb = new StringBuilder();
		for (String command : ArgumentTable.COMMANDS)
			sb.append(renderCommand(command));
		return sb.toString();
	}

	/** Renders one command's section. */
	public static String renderCommand(String command)
	{
		StringBuilder sb = new StringBuilder();
		sb.append("command ").append(command).append('\n');

		for (IArgumentDescriptor argument : ArgumentTable.describeCommand(command))
		{
			sb.append("argument\t")
				.append(argument.getTemplate())
				.append('\t')
				.append(arityText(argument.getArity()))
				.append('\t')
				.append(argument.getDescription())
				.append('\n');
		}

		return sb.toString();
	}

	/**
	 * The same surface as JSON, for {@code help --json}.
	 *
	 * <p>A separate renderer rather than a reformatting of {@link #render}: that output
	 * is deliberately line-oriented and free of braces, so an argument named
	 * {@code --json} could not honestly share it. Both read the same argument table,
	 * which is what keeps them in agreement.
	 */
	public static String renderJson()
	{
		JsonWriter writer = new JsonWriter();

		writer.beginArray();
		for (String command : ArgumentTable.COMMANDS)
		{
			writer.beginObject();
			writer.property("command", command);

			writer.name("arguments");
			writer.beginArray();
			for (IArgumentDescriptor argument : ArgumentTable.describeCommand(command))
			{
				writer.beginObject();
				writer.property("template", argument.getTemplate());
				writer.property("description", argument.getDescription());
				writer.property("arity", arityText(argument.getArity()));
				writer.endObject();
			}
			writer.endArray();

			writer.endObject();
		}
		writer.endArray();

		return writer.toString();
	}

	private static String arityText(ArgumentArity arity)
	{
		switch (arity)
		{
			case NO_VALUE:
				return "none";
			case SINGLE_VALUE:
				return "single";
			case MULTIPLE_VALUE:
				return "multiple";
			default:
				throw new IllegalArgumentException("arity");
		}
	}
}
