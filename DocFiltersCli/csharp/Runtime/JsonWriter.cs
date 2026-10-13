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

using System.Globalization;
using System.Text;

namespace Hyland.DocumentFilters.Cli.Runtime
{
	/// <summary>
	/// A minimal JSON writer, hand-rolled so that no library's escaping policy reaches
	/// the contract.
	/// </summary>
	/// <remarks>
	/// Only used for the <c>--json</c> listings on <c>list</c>, <c>identify</c>,
	/// <c>meta</c>, <c>formats</c> and <c>options</c>. Note that
	/// <c>convert --to json</c> is a completely different thing: that is the engine's
	/// own JSON canvas, and its bytes are streamed through untouched.
	///
	/// <b>Why this is not <c>System.Text.Json</c>.</b> Two of the four languages have no
	/// JSON writer to reach for: the JDK has never shipped one (JEP 198 was proposed and
	/// never delivered) and the C++17 standard library has none, so using the native
	/// writer would mean a new product dependency in half the ports. And the two that do
	/// have one disagree by default — <c>System.Text.Json</c> escapes &lt;, &gt;, &amp;,
	/// + and all non-ASCII unless given a relaxed encoder, while Python's
	/// <c>json.dumps</c> defaults to <c>ensure_ascii=True</c>. The rule below says
	/// non-ASCII is written literally, which is the default neither of them gives. D1
	/// applied to JSON rather than to error text: the contract is ours, so the writer
	/// is ours.
	///
	/// Output rules, which the Python port must reproduce even though it has a JSON
	/// module available:
	/// <list type="bullet">
	/// <item>two-space indent, one key per line</item>
	/// <item>keys in insertion order, never sorted</item>
	/// <item>UTF-8 with no byte order mark, LF line endings</item>
	/// <item>non-ASCII written literally, not escaped as <c>\uXXXX</c></item>
	/// <item>a trailing newline at the end of the document</item>
	/// </list>
	/// </remarks>
	public sealed class JsonWriter
	{
		private enum ContainerKind
		{
			Object,
			Array
		}

		private sealed class Frame
		{
			public ContainerKind Kind;
			public bool HasChildren;
		}

		private readonly StringBuilder _text = new StringBuilder();
		private readonly Stack<Frame> _open = new Stack<Frame>();

		/// <summary>
		/// True right after <see cref="Name"/>, so the next token written is that
		/// property's value rather than a new array element or sibling property —
		/// its comma and indent were already written by <see cref="Name"/> itself.
		/// </summary>
		private bool _atPropertyValue;

		/// <summary>Starts an object. Must be matched by <see cref="EndObject"/>.</summary>
		public JsonWriter BeginObject() => BeginContainer(ContainerKind.Object, '{');

		public JsonWriter EndObject() => EndContainer('}');

		/// <summary>Starts an array. Must be matched by <see cref="EndArray"/>.</summary>
		public JsonWriter BeginArray() => BeginContainer(ContainerKind.Array, '[');

		public JsonWriter EndArray() => EndContainer(']');

		/// <summary>Writes a property name, which the next value belongs to.</summary>
		public JsonWriter Name(string name)
		{
			BeginItem();
			_text.Append('"').Append(Escape(name)).Append("\": ");
			_atPropertyValue = true;
			return this;
		}

		/// <summary>Writes a string value, or null.</summary>
		public JsonWriter Value(string value)
		{
			BeforeValue();
			_text.Append(value == null ? "null" : "\"" + Escape(value) + "\"");
			return this;
		}

		public JsonWriter Value(long value)
		{
			BeforeValue();
			_text.Append(value.ToString(CultureInfo.InvariantCulture));
			return this;
		}

		public JsonWriter Value(bool value)
		{
			BeforeValue();
			_text.Append(value ? "true" : "false");
			return this;
		}

		/// <summary>Convenience for a name and string value in one call.</summary>
		public JsonWriter Property(string name, string value) => Name(name).Value(value);

		public JsonWriter Property(string name, long value) => Name(name).Value(value);

		public JsonWriter Property(string name, bool value) => Name(name).Value(value);

		/// <summary>The finished document, ending with a newline.</summary>
		/// <exception cref="InvalidOperationException">An object or array was left open.</exception>
		public override string ToString()
		{
			if (_open.Count != 0)
				throw new InvalidOperationException("a JSON object or array was left open");

			return _text.ToString() + "\n";
		}

		/// <summary>Escapes a string the way <see cref="Value(string)"/> does. Exposed for tests.</summary>
		public static string Escape(string value)
		{
			var escaped = new StringBuilder(value.Length);
			foreach (char c in value)
			{
				switch (c)
				{
					case '"':
						escaped.Append("\\\"");
						break;
					case '\\':
						escaped.Append("\\\\");
						break;
					case '\n':
						escaped.Append("\\n");
						break;
					case '\r':
						escaped.Append("\\r");
						break;
					case '\t':
						escaped.Append("\\t");
						break;
					default:
						if (c < 0x20)
							escaped.Append("\\u").Append(((int)c).ToString("x4", CultureInfo.InvariantCulture));
						else
							escaped.Append(c);
						break;
				}
			}
			return escaped.ToString();
		}

		/// <summary>
		/// Opens a new container as either a property's value or an array element,
		/// which is what <see cref="BeforeValue"/> also does for a plain value — the
		/// two are unified here since a nested object or array is just another kind
		/// of value.
		/// </summary>
		private JsonWriter BeginContainer(ContainerKind kind, char openToken)
		{
			BeforeValue();
			_text.Append(openToken);
			_open.Push(new Frame { Kind = kind });
			return this;
		}

		private JsonWriter EndContainer(char closeToken)
		{
			Frame frame = _open.Pop();
			if (frame.HasChildren)
			{
				_text.Append('\n');
				_text.Append(' ', _open.Count * 2);
			}
			_text.Append(closeToken);
			return this;
		}

		/// <summary>
		/// Writes the comma-and-newline separator and this level's indent before a
		/// new array element or object property, unless the value is a property's
		/// own value — <see cref="Name"/> already placed that punctuation.
		/// </summary>
		private void BeforeValue()
		{
			if (!_atPropertyValue)
				BeginItem();
			
			_atPropertyValue = false;
		}

		/// <summary>Marks the current container as non-empty and writes the separator before its next entry.</summary>
		private void BeginItem()
		{
			if (_open.Count == 0)
				return; // the document's own root value; nothing to separate it from

			Frame frame = _open.Peek();
			_text.Append(frame.HasChildren ? ",\n" : "\n");
			_text.Append(' ', _open.Count * 2);
			frame.HasChildren = true;
		}
	}
}
