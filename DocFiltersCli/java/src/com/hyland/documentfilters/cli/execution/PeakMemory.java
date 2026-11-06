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

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.lang.management.ManagementFactory;
import java.lang.management.MemoryPoolMXBean;
import java.nio.charset.StandardCharsets;

/**
 * The process's peak memory use, in bytes, for {@code --log-memory} and
 * {@code --max-memory}.
 *
 * <p>The reference reads {@code Process.PeakWorkingSet64}, which includes the
 * engine's native allocations. Java has no API for that. On Linux the kernel's
 * own figure, {@code VmHWM} in {@code /proc/self/status}, is the same measure and
 * is used when it exists. Elsewhere the fallback is the sum of the JVM's memory
 * pools' peaks, which sees only the JVM's own heap and metaspace: a lower bound,
 * so {@code --max-memory} fires late rather than early, never spuriously.
 */
final class PeakMemory
{
	private static final File STATUS = new File("/proc/self/status");

	private PeakMemory()
	{
	}

	static long bytes()
	{
		long fromKernel = readHighWaterMark();
		if (fromKernel >= 0)
			return fromKernel;

		long total = 0;
		for (MemoryPoolMXBean pool : ManagementFactory.getMemoryPoolMXBeans())
		{
			if (pool.getPeakUsage() != null)
				total += pool.getPeakUsage().getUsed();
		}
		return total;
	}

	/** {@code VmHWM} in bytes, or -1 where there is no such figure to read. */
	private static long readHighWaterMark()
	{
		if (!STATUS.isFile())
			return -1;

		try (BufferedReader reader = new BufferedReader(new InputStreamReader(new FileInputStream(STATUS), StandardCharsets.US_ASCII)))
		{
			String line;
			while ((line = reader.readLine()) != null)
			{
				if (!line.startsWith("VmHWM:"))
					continue;

				String digits = line.substring("VmHWM:".length()).replace("kB", "").trim();
				return Long.parseLong(digits) * 1024;
			}
		}
		catch (IOException | NumberFormatException e)
		{
			return -1;
		}
		return -1;
	}
}
