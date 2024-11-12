/*
# (c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "DocumentFiltersObjects.h"
#include "DocFiltersCommon.h"

namespace Hyland
{
	namespace DocFilters
	{
		Word::Word(const IGR_Page_Word& Word, size_t index)
			: m_word(Word), m_index(index)
		{
		}
		std::wstring Word::getText() const
		{
			return u16_to_w(m_word.word, m_word.wordLength);
		}
		int32_t Word::getX() const
		{
			return m_word.x;
		}
		int32_t Word::getY() const
		{
			return m_word.y;
		}
		int32_t Word::getWidth() const
		{
			return m_word.width;
		}
		int32_t Word::getHeight() const
		{
			return m_word.height;
		}
		int32_t Word::getRight() const
		{
			return m_word.x + m_word.width;
		}
		int32_t Word::getBottom() const
		{
			return m_word.y + m_word.height;
		}
		RectI32 Word::getRect() const
		{
			return RectI32::xywh(m_word.x, m_word.y, m_word.width, m_word.height);
		}
		size_t Word::getCharacterOffset() const
		{
			return m_word.charoffset;
		}
		size_t Word::getWordIndex() const
		{
			return m_index;
		}
		const IGR_Page_Word* Word::data() const
		{
			return &m_word;
		}
	} // namespace DocFilters
} // namespace Hyland
