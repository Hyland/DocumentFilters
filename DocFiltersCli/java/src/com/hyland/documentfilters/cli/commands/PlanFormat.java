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

package com.hyland.documentfilters.cli.commands;

import com.hyland.documentfilters.cli.discovery.InputSource;
import java.util.ArrayList;
import java.util.List;

/**
 * The handful of rendering rules every command's {@code describePlan} shares,
 * kept in one place so the conformance corpus's vocabulary --- "yes"/"no",
 * "none" for an absent value --- is spelled the same way everywhere.
 */
final class PlanFormat
{
	private PlanFormat()
	{
	}

	static String yesNo(boolean value)
	{
		return value ? "yes" : "no";
	}

	static String noneOr(String value)
	{
		return value == null || value.isEmpty() ? "none" : value;
	}

	static String noneOr(List<String> values)
	{
		return values.isEmpty() ? "none" : String.join(", ", values);
	}

	static String inputs(List<InputSource> inputs)
	{
		List<String> paths = new ArrayList<>();
		for (InputSource input : inputs)
			paths.add(input.path);
		return "inputs " + String.join(", ", paths);
	}
}
