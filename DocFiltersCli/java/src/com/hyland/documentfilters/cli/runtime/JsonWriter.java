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

package com.hyland.documentfilters.cli.runtime;

import java.util.ArrayDeque;
import java.util.Deque;

/**
 * A minimal JSON writer, hand-rolled so that no library's escaping policy reaches
 * the contract.
 *
 * <p>Only used for the {@code --json} listings on {@code list}, {@code identify},
 * {@code meta}, {@code formats} and {@code options}. Note that
 * {@code convert --to json} is a completely different thing: that is the engine's
 * own JSON canvas, and its bytes are streamed through untouched.
 *
 * <p>The JDK has never shipped a JSON writer, so in Java this is the only choice
 * as well as the contract's. Output rules:
 * <ul>
 * <li>two-space indent, one key per line</li>
 * <li>keys in insertion order, never sorted</li>
 * <li>UTF-8 with no byte order mark, LF line endings</li>
 * <li>non-ASCII written literally, not escaped as {@code \}{@code uXXXX}</li>
 * <li>an unpaired surrogate escaped as {@code \}{@code uXXXX} --- the single
 * exception to the rule above, because an unpaired surrogate is not a character
 * and has no UTF-8 encoding to write literally</li>
 * <li>a trailing newline at the end of the document</li>
 * </ul>
 */
public final class JsonWriter
{
	private enum ContainerKind
	{
		OBJECT,
		ARRAY
	}

	private static final class Frame
	{
		final ContainerKind kind;
		boolean hasChildren;

		Frame(ContainerKind kind)
		{
			this.kind = kind;
		}
	}

	private final StringBuilder _text = new StringBuilder();
	private final Deque<Frame> _open = new ArrayDeque<>();

	/**
	 * True right after {@link #name}, so the next token written is that property's
	 * value rather than a new array element or sibling property --- its comma and
	 * indent were already written by {@link #name} itself.
	 */
	private boolean _atPropertyValue;

	/** Starts an object. Must be matched by {@link #endObject}. */
	public JsonWriter beginObject()
	{
		return beginContainer(ContainerKind.OBJECT, '{');
	}

	public JsonWriter endObject()
	{
		return endContainer('}');
	}

	/** Starts an array. Must be matched by {@link #endArray}. */
	public JsonWriter beginArray()
	{
		return beginContainer(ContainerKind.ARRAY, '[');
	}

	public JsonWriter endArray()
	{
		return endContainer(']');
	}

	/** Writes a property name, which the next value belongs to. */
	public JsonWriter name(String name)
	{
		beginItem();
		_text.append('"').append(escape(name)).append("\": ");
		_atPropertyValue = true;
		return this;
	}

	/** Writes a string value, or null. */
	public JsonWriter value(String value)
	{
		beforeValue();
		if (value == null)
			_text.append("null");
		else
			_text.append('"').append(escape(value)).append('"');
		return this;
	}

	public JsonWriter value(long value)
	{
		beforeValue();
		_text.append(value);
		return this;
	}

	public JsonWriter value(boolean value)
	{
		beforeValue();
		_text.append(value ? "true" : "false");
		return this;
	}

	/** Convenience for a name and string value in one call. */
	public JsonWriter property(String name, String value)
	{
		return name(name).value(value);
	}

	public JsonWriter property(String name, long value)
	{
		return name(name).value(value);
	}

	public JsonWriter property(String name, boolean value)
	{
		return name(name).value(value);
	}

	/**
	 * The finished document, ending with a newline.
	 *
	 * @throws IllegalStateException an object or array was left open.
	 */
	@Override
	public String toString()
	{
		if (!_open.isEmpty())
			throw new IllegalStateException("a JSON object or array was left open");

		return _text.toString() + "\n";
	}

	/** Escapes a string the way {@link #value(String)} does. Exposed for tests. */
	public static String escape(String value)
	{
		StringBuilder escaped = new StringBuilder(value.length());

		// Indexed because the surrogate rule below needs the neighbouring char. A Java
		// String is UTF-16 units like C#'s, so the reference's logic carries over.
		for (int i = 0; i < value.length(); i++)
		{
			char c = value.charAt(i);
			switch (c)
			{
			case '"':
				escaped.append("\\\"");
				break;
			case '\\':
				escaped.append("\\\\");
				break;
			case '\n':
				escaped.append("\\n");
				break;
			case '\r':
				escaped.append("\\r");
				break;
			case '\t':
				escaped.append("\\t");
				break;
			default:
				// An unpaired surrogate joins the control characters in taking the
				// six-character form. It is not a character and has no UTF-8 encoding,
				// and written literally each implementation's encoder invented its own
				// answer: U+FFFD from the reference, '?' from a Java PrintStream, and an
				// unhandled UnicodeEncodeError from Python. Escaped, all four emit the
				// same bytes.
				if (c < 0x20 || (Character.isSurrogate(c) && !isPaired(value, i)))
					escaped.append(String.format("\\u%04x", (int)c));
				else
					escaped.append(c);
				break;
			}
		}
		return escaped.toString();
	}

	/**
	 * True when the surrogate at {@code index} is one half of a well-formed pair,
	 * which is a single character and is written literally like any other
	 * non-ASCII one.
	 */
	private static boolean isPaired(String value, int index)
	{
		return Character.isHighSurrogate(value.charAt(index))
			? index + 1 < value.length() && Character.isLowSurrogate(value.charAt(index + 1))
			: index > 0 && Character.isHighSurrogate(value.charAt(index - 1));
	}

	/**
	 * Opens a new container as either a property's value or an array element,
	 * which is what {@link #beforeValue} also does for a plain value --- the two
	 * are unified here since a nested object or array is just another kind of
	 * value.
	 */
	private JsonWriter beginContainer(ContainerKind kind, char openToken)
	{
		beforeValue();
		_text.append(openToken);
		_open.push(new Frame(kind));
		return this;
	}

	private JsonWriter endContainer(char closeToken)
	{
		Frame frame = _open.pop();
		if (frame.hasChildren)
		{
			_text.append('\n');
			indent();
		}
		_text.append(closeToken);
		return this;
	}

	/**
	 * Writes the comma-and-newline separator and this level's indent before a new
	 * array element or object property, unless the value is a property's own value
	 * --- {@link #name} already placed that punctuation.
	 */
	private void beforeValue()
	{
		if (!_atPropertyValue)
			beginItem();

		_atPropertyValue = false;
	}

	/** Marks the current container as non-empty and writes the separator before its next entry. */
	private void beginItem()
	{
		if (_open.isEmpty())
			return; // the document's own root value; nothing to separate it from

		Frame frame = _open.peek();
		_text.append(frame.hasChildren ? ",\n" : "\n");
		indent();
		frame.hasChildren = true;
	}

	private void indent()
	{
		for (int i = 0; i < _open.size() * 2; i++)
			_text.append(' ');
	}
}
