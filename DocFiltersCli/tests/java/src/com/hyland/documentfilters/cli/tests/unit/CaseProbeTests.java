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
import static com.hyland.documentfilters.cli.tests.Assert.assertFalse;
import static com.hyland.documentfilters.cli.tests.Assert.assertNotEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertNull;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.runtime.CaseProbe;
import com.hyland.documentfilters.cli.tests.Test;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

/**
 * Where the case question is asked, which is the part a real filesystem cannot
 * check: on one volume every directory answers the same, so a probe aimed at the
 * parent passes on every machine until someone converts into a mounted volume.
 */
public final class CaseProbeTests
{
	/**
	 * Every path the probe asks about lies inside the directory being measured.
	 * Aimed at the parent, an output under a mounted volume is measured against the
	 * filesystem holding the mount point, and two outputs differing only by case are
	 * planned as two files that turn out to be one.
	 */
	@Test
	public void asksOnlyAboutNamesInsideTheDirectory()
	{
		List<String> asked = new ArrayList<>();

		CaseProbe.measure("/mnt/ntfs", Arrays.asList("/mnt/ntfs/Report.txt"), candidate ->
		{
			asked.add(candidate);
			return false;
		});

		assertEquals(Arrays.asList("/mnt/ntfs/rEPORT.TXT"), asked);
	}

	@Test
	public void findingTheFlippedSpellingMeansCaseInsensitive()
	{
		assertFalse(CaseProbe.measure("out", Arrays.asList("out/Report.txt"), candidate -> true));
	}

	@Test
	public void notFindingItMeansCaseSensitive()
	{
		assertTrue(CaseProbe.measure("out", Arrays.asList("out/Report.txt"), candidate -> false));
	}

	/**
	 * Lazily, and only as far as it has to be: the first name with a letter in it
	 * settles the question, and a directory of a hundred thousand files costs one
	 * entry.
	 */
	@Test
	public void stopsAtTheFirstEntryThatCanAnswer()
	{
		List<String> enumerated = new ArrayList<>();

		CaseProbe.measure("out", watching(Arrays.asList("out/2026", "out/Report.txt", "out/Other.txt"), enumerated),
			candidate -> false);

		assertEquals(Arrays.asList("out/2026", "out/Report.txt"), enumerated);
	}

	@Test
	public void aDirectoryWithNothingToSayAnswersNothing()
	{
		assertNull(CaseProbe.measure("out", Collections.<String>emptyList(), candidate -> true));
		assertNull(CaseProbe.measure("out", Arrays.asList("out/2026", "out/4.1"), candidate -> true));
	}

	/** An iterable that records each entry as it is handed out, to show how far a caller read. */
	private static Iterable<String> watching(List<String> entries, List<String> seen)
	{
		return () ->
		{
			Iterator<String> inner = entries.iterator();
			return new Iterator<String>()
			{
				@Override
				public boolean hasNext()
				{
					return inner.hasNext();
				}

				@Override
				public String next()
				{
					String entry = inner.next();
					seen.add(entry);
					return entry;
				}
			};
		};
	}

	@Test
	public void probesInsideTheDirectoryBeingMeasured()
	{
		assertEquals("/mnt/ntfs/out/rEPORT.TXT", CaseProbe.flippedSibling("/mnt/ntfs/out", "/mnt/ntfs/out/Report.txt"));
	}

	/**
	 * The defect this replaced: it flipped the directory's own name and looked for
	 * it in the parent, so an output under {@code /mnt/ntfs} measured {@code /mnt}
	 * --- a different filesystem with a different answer.
	 */
	@Test
	public void neverProbesTheParent()
	{
		String probe = CaseProbe.flippedSibling("/mnt/ntfs", "/mnt/ntfs/Report.txt");

		assertTrue(probe.startsWith("/mnt/ntfs/"), probe);
		assertNotEquals("/mnt/NTFS", probe);
	}

	@Test
	public void acceptsABareNameAsWellAsAFullPath()
	{
		assertEquals("out/rEPORT.TXT", CaseProbe.flippedSibling("out", "Report.txt"));
	}

	@Test
	public void flipsEveryLetterRatherThanLowercasing()
	{
		assertEquals("out/rEPORT", CaseProbe.flippedSibling("out", "Report"));
		assertEquals("out/report", CaseProbe.flippedSibling("out", "REPORT"));
	}

	@Test
	public void leavesEverythingThatIsNotAnAsciiLetterAlone()
	{
		assertEquals("out/q4-2026_R\u00e9SUM\u00e9.pdf", CaseProbe.flippedSibling("out", "Q4-2026_r\u00e9sum\u00e9.PDF"));
	}

	/**
	 * A name with no letter to flip is the same name, which would make the probe ask
	 * whether the entry it already has exists --- always yes, and the directory would
	 * read as case-insensitive whatever it is. The caller moves on to the next entry
	 * instead.
	 */
	@Test
	public void declinesANameWithNoLetterInIt()
	{
		assertNull(CaseProbe.flippedSibling("out", "2026-01-31.4"));
		assertNull(CaseProbe.flippedSibling("out", "_"));
	}

	@Test
	public void readsATrailingSeparatorAsPartOfNeitherName()
	{
		assertEquals("out/sUB", CaseProbe.flippedSibling("out/", "out/Sub/"));
	}

	@Test
	public void readsAWindowsSeparatorTheSameWay()
	{
		assertEquals("C:/work/out/rEPORT.TXT", CaseProbe.flippedSibling("C:/work/out", "C:\\work\\out\\Report.txt"));
	}
}
