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

package com.hyland.documentfilters.cli.engine;

import java.time.LocalDateTime;

/**
 * One entry inside a container: {@code SubFile}.
 *
 * <p>Closable because the binding's subfile is a handle, not a row of data. The entry
 * owns that handle for its whole life, including the {@link IDocument} that
 * {@link #open} returns --- in the binding those are the same object, so the document
 * must not be closed separately.
 */
public interface ISubFileEntry extends AutoCloseable
{
	/** The container's own identifier for the entry. */
	String getId();

	/** The name as the container gives it --- untrusted, and never used as a path unchecked. */
	String getName();

	/** The entry's length in bytes as the container records it. */
	long getSize();

	/** The entry's timestamp, or null when the container records none. */
	LocalDateTime getModified();

	/**
	 * The entry as a document in its own right.
	 *
	 * @throws EngineException the entry could not be opened.
	 */
	IDocument open(int flags, String options);

	/** The entry's bytes, untouched. */
	void copyTo(String path);

	/** Releases the entry's handle. */
	@Override
	void close();
}
