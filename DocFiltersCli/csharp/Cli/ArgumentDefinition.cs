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

namespace Hyland.DocumentFilters.Cli.Cli
{
	/// <summary>How many values an argument takes.</summary>
	public enum ArgumentArity
	{
		/// <summary>A switch: <c>--overwrite</c>. The setter receives null.</summary>
		NoValue,

		/// <summary>One value: <c>--out-dir out/</c>. Later occurrences replace earlier ones.</summary>
		SingleValue,

		/// <summary>Repeatable: <c>--redact</c>, <c>-p</c>. The setter is called once per occurrence.</summary>
		MultipleValue
	}

	/// <summary>
	/// The parts of an argument that do not depend on what it writes into. This is the
	/// view <c>--print-contract</c> and the coverage tests use.
	/// </summary>
	public interface IArgumentDescriptor
	{
		/// <summary>McMaster-style template, e.g. <c>-o|--out-dir</c>.</summary>
		string Template { get; }

		/// <summary>Help text.</summary>
		string Description { get; }

		/// <summary>How many values it takes.</summary>
		ArgumentArity Arity { get; }

		/// <summary>The long name without dashes, e.g. <c>out-dir</c>. Used to sort the contract dump.</summary>
		string LongName { get; }

		/// <summary>The short name without its dash, e.g. <c>o</c>, or null when there is none.</summary>
		string ShortName { get; }
	}

	/// <summary>
	/// One argument, paired with the setter that writes its value into
	/// <typeparamref name="T"/>.
	/// </summary>
	/// <typeparam name="T">
	/// <c>SharedOptions</c> for a shared argument, or a per-command options class such as
	/// <c>ConvertOptions</c>.
	/// </typeparam>
	public sealed class ArgumentDefinition<T> : IArgumentDescriptor
	{
		private readonly string _longName;
		private readonly string _shortName;

		public ArgumentDefinition(string template, string description, ArgumentArity arity, Action<T, string> setter)
		{
			Template = template;
			Description = description;
			Arity = arity;
			Setter = setter;

            foreach (string form in Template.Split('|'))
            {
                if (form.StartsWith("--", StringComparison.Ordinal))
                    _longName = form.Substring(2);
				else if (form.StartsWith("-", StringComparison.Ordinal))
                    _shortName = form.Substring(1);
            }
        }

		public string Template { get; }

		public string Description { get; }

		public ArgumentArity Arity { get; }

		/// <summary>
		/// Writes the parsed value into the target. For <see cref="ArgumentArity.NoValue"/>
		/// the value argument is null.
		/// </summary>
		public Action<T, string> Setter { get; }

		/// <remarks>
		/// An argument written only in short form, such as <c>-x</c>, is named by
		/// that form — there is no long spelling to prefer.
		/// </remarks>
		public string LongName { get { return _longName ?? ShortName; } }

		public string ShortName { get { return _shortName; } }
	}
}
