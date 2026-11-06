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

import com.hyland.documentfilters.cli.configuration.FormatsOptions;
import com.hyland.documentfilters.cli.configuration.OptionCatalogOptions;
import com.hyland.documentfilters.cli.engine.EngineFormat;
import com.hyland.documentfilters.cli.engine.EngineOption;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.JsonWriter;
import java.util.ArrayList;
import java.util.List;

/**
 * The two catalogue commands: {@code formats} and {@code options}. Both ask the
 * engine what this build supports and print it; neither opens a document.
 */
public final class CatalogReports
{
	private final DocumentFiltersSession _session;
	private final IConsole _console;

	public CatalogReports(DocumentFiltersSession session, IConsole console)
	{
		_session = session;
		_console = console;
	}

	/**
	 * {@code formats} --- every readable file type.
	 *
	 * <p>There is no capability column here, unlike {@code identify}: capability
	 * flags exist only on an opened document, and nothing exposes them for a format
	 * nothing has been opened for.
	 */
	public void writeFormats(FormatsOptions options)
	{
		List<EngineFormat> formats = new ArrayList<>();
		for (EngineFormat format : _session.getApi().getSupportedFormats())
		{
			if (matchesFilter(options.filter, format.displayName, format.shortName))
				formats.add(format);
		}

		if (options.json)
		{
			JsonWriter json = new JsonWriter().beginArray();
			for (EngineFormat format : formats)
			{
				json.beginObject()
					.property("id", format.id)
					.property("ext", format.shortName)
					.property("name", format.displayName)
					.endObject();
			}
			_console.write(json.endArray().toString());
		}
		else
		{
			for (EngineFormat format : formats)
				_console.writeLine("id=" + format.id + " ext=" + text(format.shortName) + " name=" + text(format.displayName));
		}
	}

	/**
	 * {@code options} --- every engine tunable. These are the keys accepted by
	 * {@code --options}.
	 *
	 * <p>{@code applies-to} comes from the option's flags: bit {@code 0x1} means
	 * document, {@code 0x2} means canvas, as in doc2text's own
	 * {@code --options --json} dump.
	 */
	public void writeOptions(OptionCatalogOptions options)
	{
		List<EngineOption> matches = new ArrayList<>();
		List<String> scopes = new ArrayList<>();
		for (EngineOption option : _session.getApi().getAvailableOptions())
		{
			if (!matchesFilter(options.filter, option.displayName, option.description))
				continue;

			List<String> appliesTo = appliesTo(option.flags);
			if (options.appliesTo != null && !containsIgnoreCase(appliesTo, options.appliesTo))
				continue;

			matches.add(option);
			scopes.add(appliesTo.isEmpty() ? "none" : String.join(",", appliesTo));
		}

		if (options.json)
		{
			JsonWriter json = new JsonWriter().beginArray();
			for (int i = 0; i < matches.size(); i++)
			{
				EngineOption option = matches.get(i);
				json.beginObject()
					.property("name", option.displayName)
					.property("type", option.type)
					.property("default", option.defaultValue)
					.property("appliesTo", scopes.get(i))
					.property("description", option.description)
					.endObject();
			}
			_console.write(json.endArray().toString());
		}
		else
		{
			for (int i = 0; i < matches.size(); i++)
			{
				EngineOption option = matches.get(i);
				_console.writeLine(
					"name=" + text(option.displayName) + " type=" + text(option.type) + " default=" + text(option.defaultValue)
					+ " applies-to=" + scopes.get(i) + " description=" + text(option.description));
			}
		}
	}

	private static List<String> appliesTo(int flags)
	{
		List<String> scopes = new ArrayList<>();
		if ((flags & 0x1) != 0)
			scopes.add("document");
		if ((flags & 0x2) != 0)
			scopes.add("canvas");
		return scopes;
	}

	private static boolean containsIgnoreCase(List<String> values, String wanted)
	{
		for (String value : values)
		{
			if (value.equalsIgnoreCase(wanted))
				return true;
		}
		return false;
	}

	private static boolean matchesFilter(String filter, String... fields)
	{
		if (filter == null || filter.isEmpty())
			return true;

		for (String field : fields)
		{
			if (field != null && indexOfIgnoreCase(field, filter) >= 0)
				return true;
		}
		return false;
	}

	/** Where {@code needle} first occurs in {@code haystack}, ignoring case, or -1. */
	static int indexOfIgnoreCase(String haystack, String needle)
	{
		for (int i = 0; i + needle.length() <= haystack.length(); i++)
		{
			if (haystack.regionMatches(true, i, needle, 0, needle.length()))
				return i;
		}
		return -1;
	}

	/** {@code value}, or empty for null, as the reference's string interpolation renders it. */
	private static String text(String value)
	{
		return value != null ? value : "";
	}
}
