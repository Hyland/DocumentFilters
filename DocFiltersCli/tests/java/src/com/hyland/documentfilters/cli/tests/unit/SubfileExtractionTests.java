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

import static com.hyland.documentfilters.cli.tests.Assert.assertContains;
import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.execution.SubfileExtraction;
import com.hyland.documentfilters.cli.tests.Test;

/**
 * Containment of extracted subfile names.
 *
 * <p>A subfile's name comes from the container, not from the person running the
 * tool. An archive can carry {@code ../../../etc/passwd}, an absolute path, or a
 * Windows drive letter, and a tool that joins that name onto the output directory
 * writes wherever the archive says --- the vulnerability usually called zip slip.
 *
 * <p>Everything here is a pure string operation, so the whole attack surface is
 * testable without an archive, a document or a filesystem.
 */
public final class SubfileExtractionTests
{
	private static String safe(String name)
	{
		return safe(name, true);
	}

	private static String safe(String name, boolean replicateTree)
	{
		return SubfileExtraction.resolveSafeOutputPath("/out", name, replicateTree);
	}

	private static void assertInsideRoot(String resolved)
	{
		String normalised = resolved.replace('\\', '/');

		assertTrue(normalised.startsWith("/out/"), "'" + resolved + "' is not under /out/");

		// Traversal is a path *component* equal to "..", not any occurrence of the
		// two characters. Rejecting the substring would also reject the legitimate
		// filename "..data", which the test below requires to survive.
		for (String component : normalised.split("/", -1))
		{
			assertTrue(!component.equals(".."), "'" + resolved + "' contains a traversal component");
			assertTrue(!component.equals("."), "'" + resolved + "' contains an unresolved '.' component");
		}
	}

	private static void assertAllInsideRoot(String... names)
	{
		for (String name : names)
			assertInsideRoot(safe(name));
	}

	// ------------------------------------------------------------ ordinary names

	@Test
	public void anOrdinaryNameLandsDirectlyInTheOutputDirectory()
	{
		assertEquals("/out/readme.txt", safe("readme.txt").replace('\\', '/'));
	}

	@Test
	public void aNestedNameReplicatesItsStructure()
	{
		assertEquals("/out/reports/q1.docx", safe("reports/q1.docx").replace('\\', '/'));
	}

	@Test
	public void flatModeKeepsOnlyTheFinalComponent()
	{
		assertEquals("/out/q1.docx", safe("reports/q1.docx", false).replace('\\', '/'));
	}

	// ---------------------------------------------------------------- traversal

	@Test
	public void aTraversingNameCannotEscapeTheOutputDirectory()
	{
		assertAllInsideRoot(
			"../outside.txt",
			"../../outside.txt",
			"../../../../../../etc/passwd",
			"reports/../../outside.txt",
			"./../outside.txt",
			"a/b/../../../outside.txt");
	}

	@Test
	public void backslashTraversalIsCaughtToo()
	{
		// An archive written on Windows uses backslashes, and a tool that only looked
		// for forward slashes would let those through on Linux.
		assertAllInsideRoot(
			"..\\outside.txt",
			"..\\..\\windows\\system32\\evil.dll",
			"reports\\..\\..\\outside.txt");
	}

	@Test
	public void anAbsoluteNameIsTakenAsRelative()
	{
		assertAllInsideRoot(
			"/etc/passwd",
			"/absolute/path.txt",
			"\\windows\\system32\\evil.dll");
	}

	@Test
	public void aDriveOrShareQualifiedNameIsTakenAsRelative()
	{
		assertAllInsideRoot(
			"C:\\windows\\evil.dll",
			"C:/windows/evil.dll",
			"\\\\server\\share\\evil.dll");
	}

	@Test
	public void traversalIsNeutralisedInFlatModeAsWell()
	{
		// Flat mode reduces to the final component, which happens to defuse most
		// traversal --- but a name that is *only* traversal must not slip through.
		assertInsideRoot(safe("../../outside.txt", false));
	}

	// -------------------------------------------------------------- odd names

	@Test
	public void aDegenerateNameStillProducesAPathInsideTheRoot()
	{
		assertAllInsideRoot("..", ".", "...", "", "   ");
	}

	@Test
	public void aNameThatIsOnlySeparatorsProducesAPathInsideTheRoot()
	{
		assertInsideRoot(safe("///"));
	}

	@Test
	public void aDirectoryNamedDotDotSomethingIsNotTraversalAndSurvives()
	{
		// "..data" is a legitimate filename; only the "." and ".." components are
		// traversal, and an over-eager filter would corrupt valid names.
		assertContains("..data", safe("..data/x.txt"));
		assertInsideRoot(safe("..data/x.txt"));
	}

	@Test
	public void twoSubfilesWithTheSameNameResolveToTheSamePath()
	{
		// Containment does not deduplicate: overwriting is the extract command's
		// business, through --overwrite. This just records that the guard is not
		// silently renaming things.
		assertEquals(safe("readme.txt"), safe("readme.txt"));
	}
}
