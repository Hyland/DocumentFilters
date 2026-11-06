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

package com.hyland.documentfilters.cli.tests.conformance;

import static com.hyland.documentfilters.cli.tests.Assert.assertContains;
import static com.hyland.documentfilters.cli.tests.Assert.assertDoesNotContain;
import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.cli.ArgumentTable;
import com.hyland.documentfilters.cli.cli.ContractDump;
import com.hyland.documentfilters.cli.cli.IArgumentDescriptor;
import com.hyland.documentfilters.cli.tests.Test;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * {@code --print-contract}: the tool's own account of its command line surface.
 *
 * <p>A debugging aid and the machine-readable answer to {@code help --json}, so what
 * is tested here is that it faithfully reflects the argument table.
 */
public final class ContractDumpTests
{
	@Test
	public void everyVerbAppearsInTheDump()
	{
		String dump = ContractDump.render();
		for (String command : ArgumentTable.COMMANDS)
			assertContains("command " + command, dump);
	}

	@Test
	public void everyArgumentOnEveryCommandAppearsInTheDump()
	{
		for (String command : ArgumentTable.COMMANDS)
		{
			String section = ContractDump.renderCommand(command);
			for (IArgumentDescriptor argument : ArgumentTable.describeCommand(command))
			{
				assertTrue(section.contains("--" + argument.getLongName()),
					command + ": --" + argument.getLongName() + " missing from the dump");
			}
		}
	}

	@Test
	public void theDumpDeclaresNothingTheArgumentTableDoesNotHave()
	{
		// Without this a stale line in the dump would send the other ports chasing an
		// argument that no longer exists.
		for (String command : ArgumentTable.COMMANDS)
		{
			Set<String> declared = new HashSet<>();
			for (IArgumentDescriptor argument : ArgumentTable.describeCommand(command))
				declared.add(argument.getLongName());
			for (String name : longNamesIn(ContractDump.renderCommand(command)))
				assertTrue(declared.contains(name), command + ": --" + name + " is in the dump but not in the argument table");
		}
	}

	@Test
	public void theDumpAgreesWithTheSharedFixture()
	{
		// Transitively true given the two tests above and the argument table's own
		// tests, but worth asserting directly: this is the pair of files a reader will
		// diff by hand when something looks wrong.
		for (FixtureCase testCase : Fixture.load("arguments.txt"))
		{
			String section = ContractDump.renderCommand(testCase.subject);
			for (String row : testCase.getAll("argument"))
			{
				String template = PlanCorpus.splitPaths(row).get(0);
				String[] forms = template.split("\\|", -1);
				String longForm = forms[forms.length - 1];
				assertTrue(
					section.contains(longForm),
					testCase.describe(longForm + " is in arguments.txt but not in the contract dump"));
			}
		}
	}

	@Test
	public void eachArgumentRecordsItsArity()
	{
		// The other ports need this to register an argument correctly: a switch
		// declared as taking a value silently swallows the next argument.
		String section = ContractDump.renderCommand("convert");
		assertContains("none", section);
		assertContains("single", section);
		assertContains("multiple", section);
	}

	@Test
	public void theDumpIsDeterministic()
	{
		assertEquals(ContractDump.render(), ContractDump.render());
	}

	/**
	 * Not "contains no brace": a description is free to mention template
	 * placeholders like {stem} or another flag's name. The property that matters is
	 * that the dump, as a whole, is not itself JSON.
	 *
	 * <p>The JDK has no JSON parser, so rather than attempt a parse this asserts the
	 * reason a parse must fail: valid JSON permits nothing but whitespace outside its
	 * one value, and no JSON value begins with a letter other than the literals
	 * {@code true}, {@code false} and {@code null}. A dump that opens with
	 * {@code command} can therefore never parse as one JSON document, unlike
	 * renderJson's output, which always does.
	 */
	@Test
	public void theDumpIsLineOrientedAndFreeOfJson()
	{
		String dump = ContractDump.render().trim();
		assertTrue(dump.startsWith("command "), "the dump does not open with a command line");
		assertTrue("[{\"-0123456789tfn".indexOf(dump.charAt(0)) < 0, "the dump opens like a JSON value");
		assertTrue(dump.indexOf('\n') > 0, "the dump is not line oriented");
	}

	@Test
	public void theDumpUsesLfLineEndings()
	{
		// Otherwise it differs between a Windows and a Linux build.
		assertDoesNotContain("\r", ContractDump.render());
	}

	// ------------------------------------------------------------- help --json

	@Test
	public void theJsonRenderingIsActuallyJson()
	{
		// The whole reason it is a separate renderer: the flat dump is asserted to be
		// something other than JSON just above, so an argument named --json could not
		// share it.
		String json = ContractDump.renderJson();
		assertTrue(json.trim().startsWith("["), "the JSON rendering does not open with '['");
		assertTrue(json.endsWith("\n"), "the JSON rendering does not end with a newline");
		assertDoesNotContain("\r", json);
	}

	@Test
	public void theJsonRenderingNamesEveryCommand()
	{
		String json = ContractDump.renderJson();
		for (String command : ArgumentTable.COMMANDS)
			assertContains("\"" + command + "\"", json);
	}

	@Test
	public void theJsonRenderingNamesEveryArgumentOnEveryCommand()
	{
		String json = ContractDump.renderJson();
		for (String command : ArgumentTable.COMMANDS)
		{
			for (IArgumentDescriptor argument : ArgumentTable.describeCommand(command))
			{
				assertTrue(json.contains("--" + argument.getLongName()),
					command + ": --" + argument.getLongName() + " missing from the JSON");
			}
		}
	}

	@Test
	public void theJsonRenderingCarriesArityAndHelpText()
	{
		// A port registering arguments from this needs the arity; a person reading it
		// wants the description. Both have to survive the rendering.
		String json = ContractDump.renderJson();
		assertContains("\"arity\"", json);
		assertContains("\"description\"", json);
		assertContains("\"template\"", json);
	}

	@Test
	public void theJsonRenderingIsDeterministic()
	{
		assertEquals(ContractDump.renderJson(), ContractDump.renderJson());
	}

	@Test
	public void theTwoRenderingsDescribeTheSameSurface()
	{
		// They share the argument table rather than the renderer, so nothing but a test
		// keeps them agreeing.
		String json = ContractDump.renderJson();
		for (String command : ArgumentTable.COMMANDS)
		{
			for (String name : longNamesIn(ContractDump.renderCommand(command)))
				assertTrue(json.contains("--" + name), "--" + name + " is in the flat dump but not in the JSON");
		}
	}

	/**
	 * Pulls the long argument names back out of a rendered section by reading only the
	 * template column, never the free-form description: a line's tab-separated fields
	 * are {@code argument}, template, arity, description, and only the template is
	 * safe to search for {@code --name} tokens. A word split over the whole line would
	 * just as happily find one inside prose that happens to mention another flag by
	 * name.
	 */
	private static List<String> longNamesIn(String section)
	{
		List<String> names = new ArrayList<>();
		for (String line : section.split("\n", -1))
		{
			String trimmed = line.trim();
			if (!trimmed.startsWith("argument"))
				continue;

			String[] fields = trimmed.split("\t", -1);
			if (fields.length < 2)
				continue;

			for (String form : fields[1].split("\\|", -1))
			{
				if (form.startsWith("--"))
					names.add(form.substring(2));
			}
		}
		return names;
	}
}
