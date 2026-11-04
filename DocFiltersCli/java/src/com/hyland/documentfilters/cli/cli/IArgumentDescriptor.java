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

/**
 * The parts of an argument that do not depend on what it writes into. This is
 * the view {@code --print-contract} and the coverage tests use.
 */
public interface IArgumentDescriptor
{
	/**
	 * Every spelling of the argument, pipe-separated, e.g. {@code -o|--out-dir}.
	 *
	 * <p>The shape is McMaster's, which this tool was once built on, and is kept
	 * because the four implementations spell their tables identically and it is the one
	 * string a reader can compare against {@code arguments.txt} by eye. It is now read
	 * only here, to derive {@link #getLongName} and {@link #getShortName}.
	 */
	String getTemplate();

	/** Help text. */
	String getDescription();

	/** How many values it takes. */
	ArgumentArity getArity();

	/** The long name without dashes, e.g. {@code out-dir}. Used to sort the contract dump. */
	String getLongName();

	/** The short name without its dash, e.g. {@code o}, or null when there is none. */
	String getShortName();
}
