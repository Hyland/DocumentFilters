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

package com.hyland.documentfilters.cli.cli;

import java.util.function.BiConsumer;

/**
 * One argument, paired with the setter that writes its value into {@code T}.
 *
 * <p>{@code T} is {@code SharedOptions} for a shared argument, or a per-command
 * options class such as {@code ConvertOptions}.
 *
 * @param <T> the options class the setter writes into.
 */
public final class ArgumentDefinition<T> implements IArgumentDescriptor
{
	private final String _template;
	private final String _description;
	private final ArgumentArity _arity;
	private final BiConsumer<T, String> _setter;
	private String _longName;
	private String _shortName;

	/**
	 * @param setter writes the parsed value into the target. For
	 *        {@link ArgumentArity#NO_VALUE} the value argument is null.
	 */
	public ArgumentDefinition(String template, String description, ArgumentArity arity, BiConsumer<T, String> setter)
	{
		_template = template;
		_description = description;
		_arity = arity;
		_setter = setter;

		int start = 0;
		while (true)
		{
			int bar = _template.indexOf('|', start);
			String form = bar < 0 ? _template.substring(start) : _template.substring(start, bar);

			if (form.startsWith("--"))
				_longName = form.substring(2);
			else if (form.startsWith("-"))
				_shortName = form.substring(1);

			if (bar < 0)
				break;
			start = bar + 1;
		}
	}

	@Override
	public String getTemplate()
	{
		return _template;
	}

	@Override
	public String getDescription()
	{
		return _description;
	}

	@Override
	public ArgumentArity getArity()
	{
		return _arity;
	}

	/**
	 * An argument written only in short form, such as {@code -x}, is named by that
	 * form --- there is no long spelling to prefer.
	 */
	@Override
	public String getLongName()
	{
		return _longName == null ? _shortName : _longName;
	}

	@Override
	public String getShortName()
	{
		return _shortName;
	}

	/** Writes {@code value} into {@code target}. */
	public void apply(T target, String value)
	{
		_setter.accept(target, value);
	}
}
