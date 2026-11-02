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
import static com.hyland.documentfilters.cli.tests.Assert.assertNotEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.InMemoryFileSystem;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * The in-memory filesystem itself. Test infrastructure needs testing too: a fake
 * that silently discards writes would make every conversion test above it
 * meaningless.
 */
public final class InMemoryFileSystemTests
{
	@Test
	public void aCreatedFileKeepsWhatWasWrittenToIt() throws IOException
	{
		InMemoryFileSystem fs = new InMemoryFileSystem();

		try (OutputStream writer = fs.createFile("out/report.txt"))
		{
			writer.write("hello".getBytes(StandardCharsets.UTF_8));
		}

		assertEquals("hello", fs.readText("out/report.txt"));
	}

	@Test
	public void aCreatedFileCanBeReadBackThroughTheInterface() throws IOException
	{
		InMemoryFileSystem fs = new InMemoryFileSystem();

		try (OutputStream writer = fs.createFile("out/report.txt"))
		{
			writer.write("hello".getBytes(StandardCharsets.UTF_8));
		}

		try (InputStream reader = fs.openRead("out/report.txt"))
		{
			assertEquals("hello", new String(readAll(reader), StandardCharsets.UTF_8));
		}
	}

	@Test
	public void creatingAFileCreatesItsParentDirectories() throws IOException
	{
		InMemoryFileSystem fs = new InMemoryFileSystem();

		fs.createFile("out/deep/nested/report.txt").close();

		assertTrue(fs.directoryExists("out/deep/nested"));
		assertTrue(fs.fileExists("out/deep/nested/report.txt"));
	}

	@Test
	public void listingReturnsImmediateChildrenOnly() throws IOException
	{
		InMemoryFileSystem fs = new InMemoryFileSystem().addFile("a/b.txt").addFile("a/c/d.txt");

		// Sorted before comparing: the fake returns children in a deliberately
		// unhelpful order so that a walk which ignores --sort cannot pass the sorting
		// tests by accident. Asserting a sequence here would be asserting that
		// anti-feature rather than the property this test is named for.
		List<String> children = new ArrayList<>(fs.listDirectory("a"));
		Collections.sort(children);

		assertEquals(Arrays.asList("a/b.txt", "a/c"), children);
	}

	@Test
	public void pathsAreNormalisedSoTestsReadTheSameOnEveryPlatform()
	{
		InMemoryFileSystem fs = new InMemoryFileSystem().addFile("a\\b.txt");

		assertTrue(fs.fileExists("a/b.txt"));
	}

	@Test
	public void aLinkIsARealDirectoryEntrySoAWalkCanFindIt() throws IOException
	{
		// Without this the walk never sees the link, and every test about following one
		// passes without exercising anything.
		InMemoryFileSystem fs = new InMemoryFileSystem().addFile("corpus/a.txt").markSymlink("corpus/link", "target");

		assertTrue(fs.listDirectory("corpus").contains("corpus/link"));
		assertTrue(fs.isSymbolicLink("corpus/link"));
	}

	@Test
	public void listingThroughALinkYieldsTheTargetsChildrenUnderTheLinkPath() throws IOException
	{
		InMemoryFileSystem fs = new InMemoryFileSystem().addFile("shared/x.docx").markSymlink("corpus/link", "shared");

		assertEquals(Arrays.asList("corpus/link/x.docx"), fs.listDirectory("corpus/link"));
	}

	@Test
	public void aLinkToAnAncestorProducesAnEndlessSupplyOfDistinctPaths() throws IOException
	{
		// The property that makes path-based cycle detection fail, reproduced in the
		// fake so the walk's identity-based detection has something real to solve.
		InMemoryFileSystem fs = new InMemoryFileSystem().addFile("corpus/a.txt").markSymlink("corpus/loop", "corpus");

		assertTrue(fs.listDirectory("corpus/loop").contains("corpus/loop/a.txt"));
		assertTrue(fs.listDirectory("corpus/loop").contains("corpus/loop/loop"));
		assertTrue(fs.listDirectory("corpus/loop/loop").contains("corpus/loop/loop/loop"));
	}

	@Test
	public void twoLinksToOneDirectoryShareAnIdentity()
	{
		InMemoryFileSystem fs = new InMemoryFileSystem()
			.addFile("shared/x.docx")
			.markSymlink("corpus/one", "shared")
			.markSymlink("corpus/two", "shared");

		assertEquals(fs.getDirectoryIdentity("corpus/one"), fs.getDirectoryIdentity("corpus/two"));
		assertNotEquals(fs.getDirectoryIdentity("corpus/one"), fs.getDirectoryIdentity("corpus"));
	}

	private static byte[] readAll(InputStream stream) throws IOException
	{
		ByteArrayOutputStream buffer = new ByteArrayOutputStream();
		byte[] chunk = new byte[4096];
		int read;

		while ((read = stream.read(chunk)) > 0)
			buffer.write(chunk, 0, read);

		return buffer.toByteArray();
	}
}
