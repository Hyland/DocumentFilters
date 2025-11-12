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
#ifndef INC_HYLAND_DOCUMENTFILTERSOBJECTS_H
#define INC_HYLAND_DOCUMENTFILTERSOBJECTS_H

#include "DocumentFilters.h"
#include <chrono>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <type_traits>

namespace Hyland
{
	namespace DocFilters
	{
		class Annotation;
		class AnnotationAction;
		class AnnotationAppearanceStream;
		class AnnotationAppearanceStreams;
		class AnnotationAztec;
		class AnnotationBarcode;
		class AnnotationBase;
		class AnnotationCode128;
		class AnnotationCode39;
		class AnnotationCode93;
		class AnnotationDatamatrix;
		class AnnotationEllipse;
		class AnnotationFreeText;
		class AnnotationGS1_128;
		class AnnotationHighlight;
		class AnnotationInk;
		class AnnotationLine;
		class AnnotationLink;
		class AnnotationMarkup;
		class AnnotationNamedDestination;
		class AnnotationNote;
		class AnnotationNote;
		class AnnotationPDF417;
		class AnnotationPolygon;
		class AnnotationPolyline;
		class AnnotationPopup;
		class AnnotationQrCode;
		class AnnotationRectangle;
		class AnnotationSerializable;
		class AnnotationSquiggly;
		class AnnotationStamp;
		class AnnotationStickyNote;
		class AnnotationStrikeOut;
		class AnnotationUnderline;
		class Bookmark;
		class Canvas;
		class CompareDocumentSource;
		class CompareResults;
		class CompareSettings;
		class CompareDocumentSettings;
		class CompareDocumentSource;
		class CompareResultDifferenceDetail;
		class CompareResultDifference;
		class DateTime;
		class Extractor;
		class FormElement;
		class Hyperlink;
		class Format;
		class Option;
		class OcrImage;
		class OcrStyleInfo;
		class Page;
		class PageElement;
		class PagePixels;
		class Point;
		class RenderPageProperties;
		class Stream;
		class Subfile;
		class Word;
		struct Color;
		struct AnnoBind;
		class JsonWriter;
		

		/// @brief Converts a UTF-16 encoded string to a wide string.
		/// @param str The UTF-16 encoded string.
		/// @param str_len The length of the string. Defaults to std::wstring::npos.
		/// @return A wide string representation of the input UTF-16 string.
		std::wstring u16_to_w(const char16_t* str, size_t str_len = std::wstring::npos);

		/// @brief Converts a UTF-16 encoded string to a wide string.
		/// @param str The UTF-16 encoded string.
		/// @return A wide string representation of the input UTF-16 string.
		inline std::wstring u16_to_w(const std::u16string& str) { return u16_to_w(str.c_str(), str.size()); }

		/// @brief Converts a UTF-16 encoded string to a wide string.
		/// @param str The UTF-16 encoded string.
		/// @param str_len The length of the string. Defaults to std::wstring::npos.
		/// @return A wide string representation of the input UTF-16 string.
		inline std::wstring u16_to_w(const IGR_UCS2* str, size_t str_len = std::wstring::npos) { return u16_to_w(reinterpret_cast<const char16_t*>(str), str_len); }

		/// @brief Converts a UTF-16 encoded string to a wide string.
		/// @param str The UTF-16 encoded string.
		/// @return A wide string representation of the input UTF-16 string.
		template <size_t N> inline std::wstring u16_to_w(const char16_t(&str)[N]) { return u16_to_w(str, N - 1); }

		/// @brief Converts a UTF-16 encoded string to a wide string.
		/// @param str The UTF-16 encoded string.
		/// @return A wide string representation of the input UTF-16 string.
		template <size_t N> inline std::wstring u16_to_w(const IGR_UCS2(&str)[N]) { return u16_to_w(str, N - 1); }

		/// @brief Converts a UTF-16 encoded string to a UTF-8 encoded string.
		/// @param str The UTF-16 encoded string.
		/// @param str_len The length of the string. Defaults to std::wstring::npos.
		/// @return A UTF-8 encoded string representation of the input UTF-16 string.
		std::string u16_to_u8(const char16_t* str, size_t str_len = std::wstring::npos);

		/// @brief Converts a UTF-16 encoded string to a UTF-8 encoded string.
		/// @param str The UTF-16 encoded string.
		/// @return A UTF-8 encoded string representation of the input UTF-16 string.
		inline std::string u16_to_u8(const std::u16string& str) { return u16_to_u8(str.c_str(), str.size()); }

		/// @brief Converts a UTF-16 encoded string to a UTF-8 encoded string.
		/// @param str The UTF-16 encoded string.
		/// @param str_len The length of the string. Defaults to std::wstring::npos.
		/// @return A UTF-8 encoded string representation of the input UTF-16 string.
		inline std::string u16_to_u8(const IGR_UCS2* str, size_t str_len = std::wstring::npos) { return u16_to_u8(reinterpret_cast<const char16_t*>(str), str_len); }

		/// @brief Converts a UTF-16 encoded string to a UTF-8 encoded string.
		/// @param str The UTF-16 encoded string.
		/// @return A UTF-8 encoded string representation of the input UTF-16 string.
		template <size_t N> inline std::string u16_to_u8(const char16_t(&str)[N]) { return u16_to_u8(str, N - 1); }

		/// @brief Converts a UTF-16 encoded string to a UTF-8 encoded string.
		/// @param str The UTF-16 encoded string.
		/// @return A UTF-8 encoded string representation of the input UTF-16 string.
		template <size_t N> inline std::string u16_to_u8(const IGR_UCS2(&str)[N]) { return u16_to_u8(str, N - 1); }

		/// @brief Converts a UTF-8 encoded string to a wide string.
		/// @param str The UTF-8 encoded string.
		/// @param str_len The length of the string. Defaults to std::wstring::npos.
		/// @return A wide string representation of the input UTF-8 string.
		std::wstring u8_to_w(const char* str, size_t str_len = std::wstring::npos);

		/// @brief Converts a UTF-8 encoded string to a wide string.
		/// @param str The UTF-8 encoded string.
		/// @return A wide string representation of the input UTF-8 string.
		inline std::wstring u8_to_w(const std::string& str) { return u8_to_w(str.c_str(), str.size()); }

		/// @brief Converts a UTF-8 encoded string to a wide string.
		/// @param str The UTF-8 encoded string.
		/// @return A wide string representation of the input UTF-8 string.
		template <size_t N> inline std::wstring u8_to_w(const char(&str)[N]) { return u8_to_w(str, N - 1); }

		/// @brief Converts a UTF-8 encoded string to a UTF-16 encoded string.
		/// @param str The UTF-8 encoded string.
		/// @param str_len The length of the string.
		/// @return A UTF-16 encoded string representation of the input UTF-8 string.
		std::u16string u8_to_u16(const char* str, size_t str_len);

		/// @brief Converts a UTF-8 encoded string to a UTF-16 encoded string.
		/// @param str The UTF-8 encoded string.
		/// @return A UTF-16 encoded string representation of the input UTF-8 string.
		inline std::u16string u8_to_u16(const std::string& str) { return u8_to_u16(str.c_str(), str.size()); }

		/// @brief Converts a UTF-8 encoded string to a UTF-16 encoded string.
		/// @param str The UTF-8 encoded string.
		/// @return A UTF-16 encoded string representation of the input UTF-8 string.
		template <size_t N> inline std::u16string u8_to_u16(const char(&str)[N]) { return u8_to_u16(str, N - 1); }

		/// @brief Converts a wide string to a UTF-16 encoded string.
		/// @param str The wide string.
		/// @param str_len The length of the string.
		/// @return A UTF-16 encoded string representation of the input wide string.
		std::u16string w_to_u16(const wchar_t* str, size_t str_len);

		/// @brief Converts a wide string to a UTF-16 encoded string.
		/// @param str The wide string.
		/// @return A UTF-16 encoded string representation of the input wide string.
		inline std::u16string w_to_u16(const std::wstring& str) { return w_to_u16(str.c_str(), str.size()); }

		/// @brief Converts a wide string to a UTF-16 encoded string.
		/// @param str The wide string.
		/// @return A UTF-16 encoded string representation of the input wide string.
		template <size_t N> inline std::u16string w_to_u16(const wchar_t(&str)[N]) { return w_to_u16(str, N - 1); }

		/// @brief Converts a wide string to a UTF-8 encoded string.
		/// @param str The wide string.
		/// @param str_len The length of the string.
		/// @return A UTF-8 encoded string representation of the input wide string.
		std::string w_to_u8(const wchar_t* str, size_t str_len);

		/// @brief Converts a wide string to a UTF-8 encoded string.
		/// @param str The wide string.
		/// @return A UTF-8 encoded string representation of the input wide string.
		inline std::string w_to_u8(const std::wstring& str) { return w_to_u8(str.c_str(), str.size()); }

		/// @brief Converts a wide string to a UTF-8 encoded string.
		/// @param str The wide string.
		/// @return A UTF-8 encoded string representation of the input wide string.
		template <size_t N> inline std::string w_to_u8(const wchar_t(&str)[N]) { return w_to_u8(str, N - 1); }

		/// @brief Converts a wide string to a UTF-32 encoded string.
		/// @param wstr The wide string to be converted.
		/// @return A UTF-32 encoded string representation of the input wide string.
		std::u32string w_to_u32(const std::wstring& wstr);

		/// @brief Encodes a given data buffer into a Base64 string.
		/// @param data The data buffer to be encoded.
		/// @param length The length of the data buffer.
		/// @return A Base64 encoded string representation of the input data.
		std::string base64_encode(const void* data, std::size_t length);

		/// @brief Trait to enable bitmask operators for an enum.
		/// @tparam Enum The enum type.
		template<typename Enum>
		struct EnableBitMaskOperators {
			static const bool enable = false; ///< Indicates whether bitmask operators are enabled for the enum.
		};

		/// @brief Bitwise OR operator for enums.
		/// @tparam Enum The enum type.
		/// @param lhs The left-hand side enum value.
		/// @param rhs The right-hand side enum value.
		/// @return The result of the bitwise OR operation.
		template<typename Enum>
		typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
			operator|(Enum lhs, Enum rhs) {
			using underlying = typename std::underlying_type<Enum>::type;
			return static_cast<Enum>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
		}

		/// @brief Bitwise OR assignment operator for enums.
		/// @tparam Enum The enum type.
		/// @param lhs The left-hand side enum value.
		/// @param rhs The right-hand side enum value.
		/// @return The result of the bitwise OR assignment operation.
		template<typename Enum>
		typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum&>::type
			operator|=(Enum& lhs, Enum rhs) {
			lhs = lhs | rhs;
			return lhs;
		}

		/// @brief Bitwise AND operator for enums.
		/// @tparam Enum The enum type.
		/// @param lhs The left-hand side enum value.
		/// @param rhs The right-hand side enum value.
		/// @return The result of the bitwise AND operation.
		template<typename Enum>
		typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
			operator&(Enum lhs, Enum rhs) {
			using underlying = typename std::underlying_type<Enum>::type;
			return static_cast<Enum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
		}

		/// @brief Bitwise AND assignment operator for enums.
		/// @tparam Enum The enum type.
		/// @param lhs The left-hand side enum value.
		/// @param rhs The right-hand side enum value.
		/// @return The result of the bitwise AND assignment operation.
		template<typename Enum>
		typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum&>::type
			operator&=(Enum& lhs, Enum rhs) {
			lhs = lhs & rhs;
			return lhs;
		}

		/// @brief A class that provides lazy loading of elements by index.
		/// @tparam T The type of elements to be loaded.
		template <typename T>
		class lazy_loader_indexed
		{
		public:
			/// @brief An iterator for lazy_loader_indexed.
			class const_iterator
			{
			public:
				using iterator_category = std::input_iterator_tag;
				using value_type = T;
				using difference_type = std::ptrdiff_t;
				using pointer = T*;
				using reference = T&;

				/// @brief Constructs an iterator.
				/// @param loader A pointer to the lazy_loader_indexed instance.
				/// @param index The starting index for the iterator.
				const_iterator(const lazy_loader_indexed<T>* loader, size_t index)
					: m_loader(loader), m_index(index)
				{
				}

				/// @brief Pre-increment operator.
				/// @return A reference to the incremented iterator.
				const_iterator& operator++()
				{
					++m_index;
					return *this;
				}

				/// @brief Post-increment operator.
				/// @return A copy of the iterator before incrementing.
				const_iterator operator++(int)
				{
					const_iterator tmp = *this;
					++*this;
					return tmp;
				}

				/// @brief Equality comparison operator.
				/// @param other The iterator to compare with.
				/// @return True if the iterators are equal, false otherwise.
				bool operator==(const const_iterator& other) const
				{
					return m_index == other.m_index && m_loader == other.m_loader;
				}

				/// @brief Inequality comparison operator.
				/// @param other The iterator to compare with.
				/// @return True if the iterators are not equal, false otherwise.
				bool operator!=(const const_iterator& other) const
				{
					return !(*this == other);
				}

				/// @brief Dereference operator.
				/// @return The element at the current iterator position.
				T operator*() const
				{
					return m_loader->get(m_index);
				}

			private:
				const lazy_loader_indexed<T>* m_loader; ///< Pointer to the lazy_loader_indexed instance.
				size_t m_index; ///< Current index of the iterator.
			};

			/// @brief Constructs a lazy_loader_indexed instance.
			/// @param size The number of elements.
			/// @param loader A function to load elements by index.
			lazy_loader_indexed(size_t size = 0, std::function<T(size_t)> loader = nullptr)
				: m_size(size), m_loader(loader)
			{
			}

			/// @brief Gets the element at the specified index.
			/// @param index The index of the element to retrieve.
			/// @return The element at the specified index.
			/// @throws std::out_of_range if the index is out of range.
			T get(size_t index) const
			{
				if (index >= m_size)
					throw std::out_of_range("Index out of range");
				return m_loader(index);
			}

			/// @brief Indexing operator.
			/// @param index The index of the element to retrieve.
			/// @return The element at the specified index.
			T operator[](size_t index) const
			{
				return get(index);
			}

			/// @brief Gets the number of elements.
			/// @return The number of elements.
			size_t size() const
			{
				return m_size;
			}

			/// @brief Gets an iterator to the beginning of the elements.
			/// @return An iterator to the beginning of the elements.
			const_iterator begin() const
			{
				return const_iterator(this, 0);
			}

			/// @brief Gets an iterator to the end of the elements.
			/// @return An iterator to the end of the elements.
			const_iterator end() const
			{
				return const_iterator(this, m_size);
			}

		private:
			size_t m_size; ///< The number of elements.
			std::function<T(size_t)> m_loader; ///< Function to load elements by index.
		};

		/// @brief Template class for creating enumerable collections.
		/// @tparam T The type of elements in the collection.
		template <typename T>
		class enumerable_t
		{
		public:
			/// @brief Abstract class for enumerating elements in the collection.
			class enumerator_t
			{
			public:
				virtual ~enumerator_t() = default;

				/// @brief Moves to the next element in the collection.
				/// @return True if the enumerator was successfully advanced to the next element; false if the enumerator has passed the end of the collection.
				virtual bool move_next() = 0;

				/// @brief Gets the current element in the collection.
				/// @return The current element in the collection.
				virtual T current() const = 0;
			};

			/// @brief Iterator class for the enumerable collection.
			class const_iterator
			{
			public:
				using iterator_category = std::input_iterator_tag;
				using value_type = T;
				using difference_type = std::ptrdiff_t;
				using pointer = T*;
				using reference = T&;

				/// @brief Constructs an iterator for the enumerable collection.
				/// @param loader Pointer to the enumerable collection.
				/// @param enumerator Shared pointer to the enumerator.
				const_iterator(const enumerable_t* loader, std::shared_ptr<enumerator_t> enumerator)
					: m_loader(loader)
					, m_enumerator(enumerator)
				{
					if (m_enumerator && m_enumerator->move_next())
						m_current = m_enumerator->current();
					else
						m_end = true;
				}

				/// @brief Advances the iterator to the next element in the collection.
				/// @return Reference to the iterator.
				const_iterator& operator++()
				{
					if (m_enumerator->move_next())
						m_current = m_enumerator->current();
					else
					{
						m_current = T();
						m_end = true;
					}
					return *this;
				}

				/// @brief Advances the iterator to the next element in the collection (postfix increment).
				/// @return Copy of the iterator before the increment.
				const_iterator operator++(int)
				{
					const_iterator tmp = *this;
					++*this;
					return tmp;
				}

				/// @brief Checks if two iterators are equal.
				/// @param other The other iterator to compare with.
				/// @return True if the iterators are equal; otherwise, false.
				bool operator==(const const_iterator& other) const
				{
					return m_end == other.m_end
						|| (m_loader == other.m_loader && m_current == other.m_current);
				}

				/// @brief Checks if two iterators are not equal.
				/// @param other The other iterator to compare with.
				/// @return True if the iterators are not equal; otherwise, false.
				bool operator!=(const const_iterator& other) const
				{
					return !(*this == other);
				}

				/// @brief Dereferences the iterator to get the current element.
				/// @return The current element.
				T operator*() const
				{
					return m_current;
				}

				/// @brief Dereferences the iterator to get a pointer to the current element.
				/// @return Pointer to the current element.
				const T* operator->() const
				{
					return &m_current;
				}

			private:
				const enumerable_t* m_loader;
				std::shared_ptr<enumerator_t> m_enumerator;
				T m_current;
				bool m_end = false;
			};

			virtual ~enumerable_t() = default;

			/// @brief Gets an enumerator for the collection.
			/// @return Shared pointer to the enumerator.
			virtual std::shared_ptr<enumerator_t> get_enumerator() const = 0;

			/// @brief Gets an iterator to the beginning of the collection.
			/// @return Iterator to the beginning of the collection.
			const_iterator begin() const
			{
				return const_iterator(this, get_enumerator());
			}

			/// @brief Gets an iterator to the end of the collection.
			/// @return Iterator to the end of the collection.
			const_iterator end() const
			{
				return const_iterator(this, nullptr);
			}
		};

		/// @brief Enum representing the mode in which a document is opened.
		enum class OpenMode
		{
			Text,         ///< Open the document in text mode.
			Paginated,    ///< Open the document in paginated mode.
			ClassicHtml,  ///< Open the document in classic HTML mode.
		};

		/// @brief Enum representing the type of canvas used for rendering.
		enum class CanvasType
		{
			BMP = IGR_DEVICE_IMAGE_BMP,           ///< BMP image format.
			BRK = IGR_DEVICE_IMAGE_BRK,           ///< BRK image format.
			DCX = IGR_DEVICE_IMAGE_DCX,           ///< DCX image format.
			EPS = IGR_DEVICE_IMAGE_EPS,           ///< EPS image format.
			GIF = IGR_DEVICE_IMAGE_GIF,           ///< GIF image format.
			HTML = IGR_DEVICE_HTML,               ///< HTML format.
			JPEG2000 = IGR_DEVICE_IMAGE_JPEG2000, ///< JPEG2000 image format.
			JPG = IGR_DEVICE_IMAGE_JPG,           ///< JPG image format.
			JSON = IGR_DEVICE_JSON,               ///< JSON format.
			MARKDOWN = IGR_DEVICE_MARKDOWN,       ///< Markdown format.
			PBM = IGR_DEVICE_IMAGE_PBM,           ///< PBM image format.
			PCX = IGR_DEVICE_IMAGE_PCX,           ///< PCX image format.
			PDF = IGR_DEVICE_IMAGE_PDF,           ///< PDF image format.
			PGM = IGR_DEVICE_IMAGE_PGM,           ///< PGM image format.
			PNG = IGR_DEVICE_IMAGE_PNG,           ///< PNG image format.
			PPM = IGR_DEVICE_IMAGE_PPM,           ///< PPM image format.
			PS = IGR_DEVICE_IMAGE_PS,             ///< PS image format.
			SVG = IGR_DEVICE_IMAGE_SVG,           ///< SVG image format.
			TGA = IGR_DEVICE_IMAGE_TGA,           ///< TGA image format.
			TIF = IGR_DEVICE_IMAGE_TIF,           ///< TIF image format.
			WEBP = IGR_DEVICE_IMAGE_WEBP,         ///< WEBP image format.
			WEBSAFE = IGR_DEVICE_IMAGE_WEBSAFE,   ///< Web-safe image format.
			XML = IGR_DEVICE_XML,                 ///< XML format.
			XPS = IGR_DEVICE_IMAGE_XPS,           ///< XPS image format.
		};

		/// @brief Enum representing the type of pixel format used in bitmaps.
		enum class PixelType
		{
			PixelDefault = IGR_OPEN_BITMAP_PIXEL_AUTO,         ///< Default pixel format.
			Pixel1BPP = IGR_OPEN_BITMAP_PIXEL_1BPP_INDEXED,    ///< 1-bit per pixel indexed format.
			Pixel4BPP = IGR_OPEN_BITMAP_PIXEL_4BPP_INDEXED,    ///< 4-bit per pixel indexed format.
			Pixel8BPP = IGR_OPEN_BITMAP_PIXEL_8BPP_INDEXED,    ///< 8-bit per pixel indexed format.
			Pixel16BPP_565_RGB = IGR_OPEN_BITMAP_PIXEL_16BPP_565_RGB, ///< 16-bit per pixel 565 RGB format.
			Pixel16BPP_565_BGR = IGR_OPEN_BITMAP_PIXEL_16BPP_565_BGR, ///< 16-bit per pixel 565 BGR format.
			Pixel16BPP_4444_ARGB = IGR_OPEN_BITMAP_PIXEL_16BPP_4444_ARGB, ///< 16-bit per pixel 4444 ARGB format.
			Pixel16BPP_4444_BGRA = IGR_OPEN_BITMAP_PIXEL_16BPP_4444_BGRA, ///< 16-bit per pixel 4444 BGRA format.
			Pixel24BPP_888_RGB = IGR_OPEN_BITMAP_PIXEL_24BPP_888_RGB, ///< 24-bit per pixel 888 RGB format.
			Pixel24BPP_888_BGR = IGR_OPEN_BITMAP_PIXEL_24BPP_888_BGR, ///< 24-bit per pixel 888 BGR format.
			Pixel32BPP_8888_ARGB = IGR_OPEN_BITMAP_PIXEL_32BPP_8888_ARGB, ///< 32-bit per pixel 8888 ARGB format.
			Pixel32BPP_8888_BGRA = IGR_OPEN_BITMAP_PIXEL_32BPP_8888_BGRA, ///< 32-bit per pixel 8888 BGRA format.
			Pixel32BPP_8888_RGBA = IGR_OPEN_BITMAP_PIXEL_32BPP_8888_RGBA, ///< 32-bit per pixel 8888 RGBA format.
			Pixel32BPP_8888_ABGR = IGR_OPEN_BITMAP_PIXEL_32BPP_8888_ABGR, ///< 32-bit per pixel 8888 ABGR format.
		};

		/// @brief Enum representing flags for pixel properties.
		enum class PixelsFlags
		{
			MinIsBlack = IGR_OPEN_BITMAP_FLAGS_MIN_IS_BLACK, ///< Minimum value is black.
			BottomUp = IGR_OPEN_BITMAP_FLAGS_BOTTOM_UP       ///< Bitmap is stored bottom-up.
		};

		/// @brief Template class representing a rectangle with various utility functions.
		/// @tparam RectType The type of the rectangle.
		/// @tparam T The type of the coordinates.
		template <typename RectType, typename T>
		class rect_t : public RectType
		{
		public:
			/// @brief Constructs a rectangle from a given RectType object.
			/// @param rect The RectType object. Defaults to an empty RectType object.
			rect_t(const RectType rect = RectType{})
				: RectType(rect)
			{
			}

			/// @brief Constructs a rectangle from given coordinates.
			/// @param left The left coordinate.
			/// @param top The top coordinate.
			/// @param right The right coordinate.
			/// @param bottom The bottom coordinate.
			rect_t(T left, T top, T right, T bottom)
				: RectType{ left, top, right, bottom }
			{
			}

			/// @brief Gets the x-coordinate (left) of the rectangle.
			/// @return The x-coordinate.
			T x() const { return RectType::left; }

			/// @brief Gets the y-coordinate (top) of the rectangle.
			/// @return The y-coordinate.
			T y() const { return RectType::top; }

			/// @brief Gets the width of the rectangle.
			/// @return The width.
			T width() const { return RectType::right - RectType::left; }

			/// @brief Gets the height of the rectangle.
			/// @return The height.
			T height() const { return RectType::bottom - RectType::top; }

			/// @brief Equality operator.
			/// @param other The other rectangle to compare with.
			/// @return True if the rectangles are equal, false otherwise.
			bool operator==(const rect_t& other) const
			{
				return RectType::left == other.left && RectType::top == other.top && RectType::right == other.right && RectType::bottom == other.bottom;
			}

			/// @brief Inequality operator.
			/// @param other The other rectangle to compare with.
			/// @return True if the rectangles are not equal, false otherwise.
			bool operator!=(const rect_t& other) const
			{
				return !(*this == other);
			}

			/// @brief Move the rectangle by the specified offset.
			/// @param x amount to move in the x direction.
			/// @param y amount to move in the y direction.
			/// @return Reference to the rectangle.
			rect_t& offset(int x, int y) {
				this->left += x;
				this->top += y;
				this->right += x;
				this->bottom += y;
				return *this;
			}

			/// @brief Converts a rectangle to a quadrilateral point representation.
			/// @return An IGR_QuadPoint object representing the rectangle as a quadrilateral with four corner points: upper-left, upper-right, lower-right, and lower-left.
			IGR_QuadPoint to_quadpoint() const
			{
				IGR_QuadPoint quad{};
				quad.upperLeft.x = static_cast<IGR_FLOAT>(RectType::left);
				quad.upperLeft.y = static_cast<IGR_FLOAT>(RectType::top);
				quad.upperRight.x = static_cast<IGR_FLOAT>(RectType::right);
				quad.upperRight.y = static_cast<IGR_FLOAT>(RectType::top);
				quad.lowerRight.x = static_cast<IGR_FLOAT>(RectType::right);
				quad.lowerRight.y = static_cast<IGR_FLOAT>(RectType::bottom);
				quad.lowerLeft.x = static_cast<IGR_FLOAT>(RectType::left);
				quad.lowerLeft.y = static_cast<IGR_FLOAT>(RectType::bottom);
				return quad;
			}

			/// @brief Creates a rectangle from left, top, right, and bottom coordinates.
			/// @param left The left coordinate.
			/// @param top The top coordinate.
			/// @param right The right coordinate.
			/// @param bottom The bottom coordinate.
			/// @return A rectangle with the specified coordinates.
			static rect_t ltrb(T left, T top, T right, T bottom) { return rect_t(left, top, right, bottom); }

			/// @brief Creates a rectangle from x, y, width, and height.
			/// @param x The x-coordinate (left).
			/// @param y The y-coordinate (top).
			/// @param w The width.
			/// @param h The height.
			/// @return A rectangle with the specified dimensions.
			static rect_t xywh(T x, T y, T w, T h) { return ltrb(x, y, x + w, y + h); }
		};

		/// @brief Typedef for a rectangle with IGR_Rect type and IGR_ULONG coordinates.
		typedef rect_t<IGR_Rect, IGR_ULONG> RectUI32;

		/// @brief Typedef for a rectangle with IGR_SRect type and IGR_LONG coordinates.
		typedef rect_t<IGR_SRect, IGR_LONG> RectI32;

		/// @brief Typedef for a rectangle with IGR_FRect type and IGR_FLOAT coordinates.
		typedef rect_t<IGR_FRect, IGR_FLOAT> RectF;

		using Rect = RectI32;

		/// @brief Represents a date and time.
		class DateTime
		{
		private:
			std::chrono::system_clock::time_point m_time;

		public:
			/// @brief Enum to specify the getType of date.
			enum class date_type_t {
				utc,   ///< Coordinated Universal Time.
				local  ///< Local time.
			};

			/// @brief Constructs a DateTime object from a file time value.
			/// @param value The file time value.
			DateTime(IGR_ULONGLONG value = 0);

			/// @brief Constructs a DateTime object from a time point.
			/// @param time The time point.
			DateTime(const std::chrono::system_clock::time_point& time);

			/// @brief Constructs a DateTime object from individual date and time components.
			/// @param year The year.
			/// @param month The month.
			/// @param day The day.
			/// @param hour The hour. Defaults to 0.
			/// @param minute The minute. Defaults to 0.
			/// @param second The second. Defaults to 0.
			/// @param millisecond The millisecond. Defaults to 0.
			DateTime(uint16_t year, uint16_t month, uint16_t day, uint16_t hour = 0, uint16_t minute = 0, uint16_t second = 0, uint16_t millisecond = 0);

			/// @brief Constructs a DateTime object from individual date and time components with a specified date getType.
			/// @param getType The date getType (UTC or local).
			/// @param year The year.
			/// @param month The month.
			/// @param day The day.
			/// @param hour The hour. Defaults to 0.
			/// @param minute The minute. Defaults to 0.
			/// @param second The second. Defaults to 0.
			/// @param millisecond The millisecond. Defaults to 0.
			DateTime(date_type_t type, uint16_t year, uint16_t month, uint16_t day, uint16_t hour = 0, uint16_t minute = 0, uint16_t second = 0, uint16_t millisecond = 0);

			/// @brief Constructs a DateTime object from an ISO 8601 string.
			/// @param value The ISO 8601 string.
			DateTime(const std::string& value);

			/// @brief Constructs a DateTime object from a wide string representation of an ISO 8601 string.
			/// @param value The wide string representation of the ISO 8601 string.
			DateTime(const std::wstring& value);

			/// @brief Converts the DateTime object to a time point.
			/// @return The time point representation of the DateTime object.
			std::chrono::system_clock::time_point ToTimePoint() const;

			/// @brief Converts the DateTime object to an ISO 8601 string.
			/// @return The ISO 8601 string representation of the DateTime object.
			std::string ToIsoString() const;

			/// @brief Converts the internal date-time representation to a std::tm structure.
			/// @param as_utc If true, the date-time is converted to UTC. Otherwise, it is converted to local time.
			/// @return An optional std::tm structure representing the date-time. If the conversion fails, the optional is empty.
			std::optional<std::tm> ToTm(bool as_utc) const;

			/// @brief Checks if the DateTime object is empty.
			/// @return True if the DateTime object is empty, false otherwise.
			bool empty() const;

			/// @brief Equality operator.
			/// @param other The other DateTime object to compare.
			/// @return True if the DateTime objects are equal, false otherwise.
			bool operator==(const DateTime& other) const;

			/// @brief Inequality operator.
			/// @param other The other DateTime object to compare.
			/// @return True if the DateTime objects are not equal, false otherwise.
			bool operator!=(const DateTime& other) const;

			/// @brief Creates a UTC DateTime object from individual date and time components.
			/// @param year The year.
			/// @param month The month.
			/// @param day The day.
			/// @param hour The hour. Defaults to 0.
			/// @param minute The minute. Defaults to 0.
			/// @param second The second. Defaults to 0.
			/// @param millisecond The millisecond. Defaults to 0.
			/// @return The UTC DateTime object.
			static DateTime utc(uint16_t year, uint16_t month, uint16_t day, uint16_t hour = 0, uint16_t minute = 0, uint16_t second = 0, uint16_t millisecond = 0);

			/// @brief Creates a local DateTime object from individual date and time components.
			/// @param year The year.
			/// @param month The month.
			/// @param day The day.
			/// @param hour The hour. Defaults to 0.
			/// @param minute The minute. Defaults to 0.
			/// @param second The second. Defaults to 0.
			/// @param millisecond The millisecond. Defaults to 0.
			/// @return The local DateTime object.
			static DateTime local(uint16_t year, uint16_t month, uint16_t day, uint16_t hour = 0, uint16_t minute = 0, uint16_t second = 0, uint16_t millisecond = 0);

			/// @brief Converts a file time value to a time point.
			/// @param value The file time value.
			/// @return The time point representation of the file time value.
			static std::chrono::system_clock::time_point from_filetime(IGR_ULONGLONG value);
		};

		/// @brief Represents a point in 2D space.
		class Point
		{
		private:
			int m_x = 0; ///< The x-coordinate of the point.
			int m_y = 0; ///< The y-coordinate of the point.
		public:
			/// @brief Constructs a Point object with optional x and y coordinates.
			/// @param x The x-coordinate. Defaults to 0.
			/// @param y The y-coordinate. Defaults to 0.
			Point(int x = 0, int y = 0)
				: m_x(x), m_y(y)
			{
			};

			/// @brief Gets the x-coordinate of the point.
			/// @return The x-coordinate.
			int x() const { return m_x; }

			/// @brief Gets the y-coordinate of the point.
			/// @return The y-coordinate.
			int y() const { return m_y; }

			/// @brief Equality operator.
			/// @param other The other Point object to compare.
			/// @return True if the points are equal, false otherwise.
			bool operator==(const Point& other) const { return m_x == other.m_x && m_y == other.m_y; }

			/// @brief Inequality operator.
			/// @param other The other Point object to compare.
			/// @return True if the points are not equal, false otherwise.
			bool operator!=(const Point& other) const { return m_x != other.m_x || m_y != other.m_y; }
		};


		/// @brief The `DocumentFilters` class provides functionality for managing document filters, initializing with licenses and paths, and retrieving or opening extractors for various data sources.
		class DocumentFilters
		{
		public:
			/// @brief Exception class for handling errors with return codes.
			class Error : public std::runtime_error
			{
			private:
				IGR_RETURN_CODE m_code; ///< The error return code.

			public:
				/// @brief Constructs an error with a specific return code and message.
				/// @param code The return code associated with the error.
				/// @param message The error message. Defaults to an empty string.
				Error(IGR_RETURN_CODE code, const std::string& message = std::string());

				/// @brief Constructs an error with a default return code and a message.
				/// @param message The error message.
				Error(const std::string& message) : Error(IGR_E_OPEN_ERROR, message) {}

				/// @brief Retrieves the return code associated with the error.
				/// @return The return code.
				IGR_RETURN_CODE code() const { return m_code; }
			};

			typedef std::function<int(int action, void* payload)> open_callback_t;

#if defined(_WIN32) || defined(_WIN64)
			typedef void(__cdecl* memory_destruct_t)(void*);
#else
			typedef void (*memory_destruct_t)(void*);
#endif

			/// @brief Default constructor for the document_filters class.
			DocumentFilters();

			/// @brief Constructs a document_filters object with a license and path.
			/// @param license The license key as a wide string.
			/// @param path The path to the document as a wide string.
			DocumentFilters(const std::wstring& license, const std::wstring& path);

			/// @brief Constructs a document_filters object with a license and path.
			/// @param license The license key as a string.
			/// @param path The path to the document as a string.
			DocumentFilters(const std::string& license, const std::string& path);

			/// @brief Initializes the document_filters object with a license and path.
			/// @param license The license key as a wide string.
			/// @param path The path to the document as a wide string.
			void Initialize(const std::wstring& license, const std::wstring& path);

			/// @brief Initializes the document_filters object with a license and path.
			/// @param license The license key as a string.
			/// @param path The path to the document as a string.
			void Initialize(const std::string& license, const std::string& path);

			/// @brief Retrieves an extractor for the specified file.
			/// @param filename The name of the file as a string.
			/// @return An extractor object for the specified file.
			Extractor GetExtractor(const std::string& filename);

			/// @brief Retrieves an extractor for the specified file.
			/// @param filename The name of the file as a wide string.
			/// @return An extractor object for the specified file.
			Extractor GetExtractor(const std::wstring& filename);

			/// @brief Retrieves an extractor for the specified data.
			/// @param data Pointer to the data.
			/// @param size Size of the data.
			/// @param deleter Optional deleter function for the data.
			/// @return An extractor object for the specified data.
			Extractor GetExtractor(const void* data, size_t size, memory_destruct_t deleter = nullptr);

			/// @brief Retrieves an extractor for the specified stream.
			/// @param stream Pointer to the IGR_Stream object.
			/// @return An extractor object for the specified stream.
			Extractor GetExtractor(IGR_Stream* stream);

			/// @brief Retrieves an extractor for the specified input stream.
			/// @param stream Reference to the input stream.
			/// @return An extractor object for the specified input stream.
			Extractor GetExtractor(std::istream& stream);

			/// @brief Retrieves an extractor for the specified input stream.
			/// @param stream Pointer to the input stream.
			/// @param own_stream Boolean indicating ownership of the stream.
			/// @return An extractor object for the specified input stream.
			Extractor GetExtractor(std::istream* stream, bool own_stream);

			/// @brief Retrieves an extractor for the specified file.
			/// @param file Pointer to the file.
			/// @param own_file Boolean indicating ownership of the file.
			/// @return An extractor object for the specified file.
			Extractor GetExtractor(FILE* file, bool own_file);

			/// @brief Retrieves an extractor for the specified stream.
			/// @param stream Reference to the stream object.
			/// @return An extractor object for the specified stream.
			Extractor GetExtractor(Stream& stream);

			/// @brief Retrieves an extractor for the specified stream.
			/// @param stream Pointer to the stream object.
			/// @param own_stream Boolean indicating ownership of the stream.
			/// @return An extractor object for the specified stream.
			Extractor GetExtractor(Stream* stream, bool own_stream);

			/// @brief Opens an extractor for the specified file.
			/// @param filename The name of the file as a string.
			/// @param mode The mode in which to open the file.
			/// @param open_flags Optional getFlags for opening the file. Defaults to 0.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @param callback Optional callback function. Defaults to nullptr.
			/// @return An extractor object for the specified file.
			Extractor OpenExtractor(const std::string& filename, OpenMode mode, int open_flags = 0, const std::wstring& options = std::wstring(), const open_callback_t& callback = nullptr);

			/// @brief Opens an extractor for the specified file.
			/// @param filename The name of the file as a wide string.
			/// @param mode The mode in which to open the file.
			/// @param open_flags Optional getFlags for opening the file. Defaults to 0.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @param callback Optional callback function. Defaults to nullptr.
			/// @return An extractor object for the specified file.
			Extractor OpenExtractor(const std::wstring& filename, OpenMode mode, int open_flags = 0, const std::wstring& options = std::wstring(), const open_callback_t& callback = nullptr);

			/// @brief Opens an extractor for the specified data.
			/// @param data Pointer to the data.
			/// @param size Size of the data.
			/// @param deleter Function to delete the data.
			/// @param mode The mode in which to open the data.
			/// @param open_flags Optional getFlags for opening the data. Defaults to 0.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @param callback Optional callback function. Defaults to nullptr.
			/// @return An extractor object for the specified data.
			Extractor OpenExtractor(const void* data, size_t size, memory_destruct_t deleter, OpenMode mode, int open_flags = 0, const std::wstring& options = std::wstring(), const open_callback_t& callback = nullptr);

			/// @brief Opens an extractor for the specified stream.
			/// @param stream Pointer to the IGR_Stream object.
			/// @param mode The mode in which to open the stream.
			/// @param open_flags Optional getFlags for opening the stream. Defaults to 0.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @param callback Optional callback function. Defaults to nullptr.
			/// @return An extractor object for the specified stream.
			Extractor OpenExtractor(IGR_Stream* stream, OpenMode mode, int open_flags = 0, const std::wstring& options = std::wstring(), const open_callback_t& callback = nullptr);

			/// @brief Opens an extractor for the specified input stream.
			/// @param stream Reference to the input stream.
			/// @param mode The mode in which to open the stream.
			/// @param open_flags Optional getFlags for opening the stream. Defaults to 0.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @param callback Optional callback function. Defaults to nullptr.
			/// @return An extractor object for the specified input stream.
			Extractor OpenExtractor(std::istream& stream, OpenMode mode, int open_flags = 0, const std::wstring& options = std::wstring(), const open_callback_t& callback = nullptr);

			/// @brief Opens an extractor for the specified input stream.
			/// @param stream Pointer to the input stream.
			/// @param own_stream Boolean indicating ownership of the stream.
			/// @param mode The mode in which to open the stream.
			/// @param open_flags Optional getFlags for opening the stream. Defaults to 0.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @param callback Optional callback function. Defaults to nullptr.
			/// @return An extractor object for the specified input stream.
			Extractor OpenExtractor(std::istream* stream, bool own_stream, OpenMode mode, int open_flags = 0, const std::wstring& options = std::wstring(), const open_callback_t& callback = nullptr);

			/// @brief Opens an extractor for the specified file.
			/// @param file Pointer to the file.
			/// @param own_file Boolean indicating ownership of the file.
			/// @param mode The mode in which to open the file.
			/// @param open_flags Optional getFlags for opening the file. Defaults to 0.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @param callback Optional callback function. Defaults to nullptr.
			/// @return An extractor object for the specified file.
			Extractor OpenExtractor(FILE* file, bool own_file, OpenMode mode, int open_flags = 0, const std::wstring& options = std::wstring(), const open_callback_t& callback = nullptr);

			/// @brief Opens an extractor for the specified stream.
			/// @param stream Reference to the stream object.
			/// @param mode The mode in which to open the stream.
			/// @param open_flags Optional getFlags for opening the stream. Defaults to 0.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @param callback Optional callback function. Defaults to nullptr.
			/// @return An extractor object for the specified stream.
			Extractor OpenExtractor(Stream& stream, OpenMode mode, int open_flags = 0, const std::wstring& options = std::wstring(), const open_callback_t& callback = nullptr);

			/// @brief Opens an extractor for the specified stream.
			/// @param stream Pointer to the stream object.
			/// @param own_stream Boolean indicating ownership of the stream.
			/// @param mode The mode in which to open the stream.
			/// @param open_flags Optional getFlags for opening the stream. Defaults to 0.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @param callback Optional callback function. Defaults to nullptr.
			/// @return An extractor object for the specified stream.
			Extractor OpenExtractor(Stream* stream, bool own_stream, OpenMode mode, int open_flags = 0, const std::wstring& options = std::wstring(), const open_callback_t& callback = nullptr);

			/// @brief Creates an output canvas for the specified file.
			/// @param filename The name of the file as a string.
			/// @param getType The getType of the canvas.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @return A canvas object for the specified file.
			Canvas MakeOutputCanvas(const std::string& filename, CanvasType type, const std::wstring& options = std::wstring());

			/// @brief Creates an output canvas for the specified file.
			/// @param filename The name of the file as a wide string.
			/// @param getType The getType of the canvas.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @return A canvas object for the specified file.
			Canvas MakeOutputCanvas(const std::wstring& filename, CanvasType type, const std::wstring& options = std::wstring());

			/// @brief Creates an output canvas for the specified stream.
			/// @param stream Reference to the input/output stream.
			/// @param getType The getType of the canvas.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @return A canvas object for the specified stream.
			Canvas MakeOutputCanvas(std::iostream& stream, CanvasType type, const std::wstring& options = std::wstring());

			/// @brief Creates an output canvas for the specified stream.
			/// @param stream Pointer to the input/output stream.
			/// @param own_stream Boolean indicating ownership of the stream.
			/// @param getType The getType of the canvas.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @return A canvas object for the specified stream.
			Canvas MakeOutputCanvas(std::iostream* stream, bool own_stream, CanvasType type, const std::wstring& options = std::wstring());

			/// @brief Creates an output canvas for the specified file.
			/// @param file Pointer to the file.
			/// @param own_file Boolean indicating ownership of the file.
			/// @param getType The getType of the canvas.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @return A canvas object for the specified file.
			Canvas MakeOutputCanvas(FILE* file, bool own_file, CanvasType type, const std::wstring& options = std::wstring());

			/// @brief Creates an output canvas for the specified stream.
			/// @param stream Reference to the stream object.
			/// @param getType The getType of the canvas.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @return A canvas object for the specified stream.
			Canvas MakeOutputCanvas(Stream& stream, CanvasType type, const std::wstring& options = std::wstring());

			/// @brief Creates an output canvas for the specified stream.
			/// @param stream Pointer to the stream object.
			/// @param own_stream Boolean indicating ownership of the stream.
			/// @param getType The getType of the canvas.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @return A canvas object for the specified stream.
			Canvas MakeOutputCanvas(Stream* stream, bool own_stream, CanvasType type, const std::wstring& options = std::wstring());

			/// @brief Creates an output canvas for the specified writable stream.
			/// @param stream Pointer to the writable stream object.
			/// @param getType The getType of the canvas.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @return A canvas object for the specified writable stream.
			Canvas MakeOutputCanvas(IGR_Writable_Stream* stream, CanvasType type, const std::wstring& options = std::wstring());

			/// @brief Retrieves the list of available formats.
			/// @return A constant reference to a vector of `igr_format` objects representing the available formats.
			const std::vector<Format>& getFormats() const;

			/// @brief Retrieves the list of supported formats.
			/// This is an alias for `get_formats()`.
			/// @return A constant reference to a vector of `igr_format` objects representing the supported formats.
			const std::vector<Format>& getSupportedFormats() const { return getFormats(); }

			/// @brief Retrieves the list of available options.
			/// @return A constant reference to a vector of `igr_option` objects representing the available options.
			const std::vector<Option>& getOptions() const;

			/// @brief Retrieves the list of available options.
			/// This is an alias for `get_options()`.
			/// @return A constant reference to a vector of `igr_option` objects representing the available options.
			const std::vector<Option>& getAvailableOptions() const { return getOptions(); }

		protected:
			class impl_t;
			std::shared_ptr<impl_t> m_impl;

			/// @brief Creates an output canvas for the specified writable stream.
			/// @param stream Pointer to the writable stream object.
			/// @param type The type of the canvas.
			/// @param options Optional additional options as a wide string. Defaults to an empty string.
			/// @param own_stream Boolean indicating ownership of the stream.
			/// @return A canvas object for the specified writable stream.
			Canvas DoMakeOutputCanvas(IGR_Writable_Stream* stream, CanvasType type, const std::wstring& options, bool own_stream);
		};

		using Api = DocumentFilters;

		/// @brief Abstract base class representing a stream.
		class Stream
		{
		public:
			/// @brief Virtual destructor for the stream class.
			virtual ~Stream() = default;

			/// @brief Seeks to a specific position in the stream.
			/// @param offset The position to seek to.
			/// @param way The direction to seek (beginning, current, end).
			/// @return The new position in the stream.
			virtual std::streamoff seek(std::streampos offset, std::ios_base::seekdir way) = 0;

			/// @brief Reads data from the stream into a buffer.
			/// @param buffer The buffer to read data into.
			/// @param size The number of bytes to read.
			/// @return The number of bytes actually read.
			virtual size_t read(void* buffer, size_t size) = 0;

			/// @brief Writes data from a buffer to the stream.
			/// @param buffer The buffer containing data to write.
			/// @param size The number of bytes to write.
			/// @return The number of bytes actually written (default is 0).
			virtual size_t write(const void* buffer, size_t size) { return 0; }

			/// @brief Gets the size of the stream.
			/// @return The size of the stream in bytes.
			std::streamoff getSize();

			/// @brief Gets the current position in the stream.
			/// @return The current position in the stream.
			std::streamoff get_position();

			/// @brief Sets the current position in the stream.
			/// @param pos The position to set.
			/// @return The new position in the stream.
			std::streamoff set_position(std::streamoff pos);

			/// @brief Relinquishes ownership of the underlying IGR_Stream.
			/// @return A pointer to the relinquished IGR_Stream.
			virtual IGR_Stream* relinquish_igr_stream() { return nullptr; };

			/// @brief Bridges a standard iostream to an IGR_Stream.
			///
			/// This function creates a bridge between a standard iostream and an IGR_Stream.
			///
			/// @param stream A pointer to the standard iostream.
			/// @param own_stream A boolean indicating whether the function should take ownership of the stream.
			/// @param igr_stream A pointer to the IGR_Stream to be bridged.
			/// @return A pointer to the bridged IGR_Stream.
			static IGR_Stream* bridge_iostream(std::iostream* stream, bool own_stream, IGR_Stream** igr_stream);

			/// @brief Bridges a standard istream to an IGR_Stream.
			///
			/// This function creates a bridge between a standard istream and an IGR_Stream.
			///
			/// @param stream A pointer to the standard istream.
			/// @param own_stream A boolean indicating whether the function should take ownership of the stream.
			/// @param igr_stream A pointer to the IGR_Stream to be bridged.
			/// @return A pointer to the bridged IGR_Stream.
			static IGR_Stream* bridge_istream(std::istream* stream, bool own_stream, IGR_Stream** igr_stream);

			/// @brief Bridges a standard file to an IGR_Stream.
			///
			/// This function creates a bridge between a standard file and an IGR_Stream.
			///
			/// @param file A pointer to the standard file.
			/// @param own_file A boolean indicating whether the function should take ownership of the file.
			/// @param igr_stream A pointer to the IGR_Stream to be bridged.
			/// @return A pointer to the bridged IGR_Stream.
			static IGR_Stream* bridge_file(FILE* file, bool own_file, IGR_Stream** igr_stream);

			/// @brief Bridges a hyland::docfilters::stream to an IGR_Stream.
			///
			/// This function creates a bridge between a hyland::docfilters::stream and an IGR_Stream.
			///
			/// @param stream A pointer to the hyland::docfilters::stream.
			/// @param own_stream A boolean indicating whether the function should take ownership of the stream.
			/// @param igr_stream A pointer to the IGR_Stream to be bridged.
			/// @return A pointer to the bridged IGR_Stream.
			static IGR_Stream* bridge_stream(Hyland::DocFilters::Stream* stream, bool own_stream, IGR_Stream** igr_stream);

		};

		/// @brief A memory stream class derived from the stream class.
		class MemStream : public Stream
		{
		public:
			/// @brief Constructs a memory stream with a given buffer and size.
			/// @param buffer Pointer to the buffer.
			/// @param size Size of the buffer.
			MemStream(void* buffer, size_t size);

			/// @brief Constructs a memory stream with a given constant buffer and size.
			/// @param buffer Pointer to the constant buffer.
			/// @param size Size of the buffer.
			MemStream(const void* buffer, size_t size);

			/// @brief Seeks to a specific position in the memory stream.
			/// @param offset The position to seek to.
			/// @param way The direction to seek (beginning, current, end).
			/// @return The new position in the stream.
			std::streamoff seek(std::streampos offset, std::ios_base::seekdir way);

			/// @brief Reads data from the memory stream into a buffer.
			/// @param buffer The buffer to read data into.
			/// @param size The number of bytes to read.
			/// @return The number of bytes actually read.
			size_t read(void* buffer, size_t size);

			/// @brief Writes data from a buffer to the memory stream.
			/// @param buffer The buffer containing data to write.
			/// @param size The number of bytes to write.
			/// @return The number of bytes actually written.
			size_t write(const void* buffer, size_t size);

			/// @brief Gets the size of the memory stream.
			/// @return The size of the memory stream in bytes.
			size_t size() const { return m_size; }

			/// @brief Sets the size of the memory stream.
			/// @param size The new size of the memory stream.
			void set_size(size_t size);

			/// @brief Gets the memory buffer of the stream.
			/// @return A constant pointer to the memory buffer.
			const void* get_memory() const { return m_buffer; }
		protected:
			/// @brief Resizes the memory stream to a new size.
			/// @param new_size The new size of the memory stream.
			void resize(size_t new_size);

			/// @brief Resizes the buffer to a new size. Can be overridden by derived classes.
			/// @param buffer Pointer to the buffer.
			/// @param new_size The new size of the buffer.
			/// @return A pointer to the resized buffer. Default implementation returns nullptr.
			virtual void* resize_buffer(void* buffer, size_t new_size) { return nullptr; }

			void* m_buffer = nullptr; ///< Pointer to the memory buffer.
			size_t m_offset = 0; ///< Current offset in the memory stream.
			size_t m_capacity = 0; ///< Capacity of the memory buffer.
			size_t m_size = 0; ///< Size of the memory stream.
		};

		/// @brief A memory stream class that uses a vector as its underlying storage.
		class VectorStream : public MemStream
		{
		public:
			/// @brief Constructs an empty vector stream.
			VectorStream();

			/// @brief Retrieves the data stored in the vector stream.
			/// @return A constant reference to a vector of bytes representing the data.
			const std::vector<uint8_t>& data() const { return m_data; }
		protected:
			/// @brief Resizes the buffer to a new size.
			/// @param buffer Pointer to the buffer.
			/// @param new_size The new size of the buffer.
			/// @return A pointer to the resized buffer.
			void* resize_buffer(void* buffer, size_t new_size) override;
		private:
			std::vector<uint8_t> m_data; ///< Vector storing the stream data.
		};

		/// @brief A class for handling file streams, inheriting from the Stream class.
		class FileStream : public Stream
		{
		public:
			FileStream(std::istream* stream, bool own_stream);
			FileStream(std::iostream* stream, bool own_stream);
			FileStream(FILE* stream, bool own_stream);
			FileStream(const std::string& filename);
			FileStream(const std::wstring& filename);
			~FileStream() override;

			/// @brief Seeks to a specific position in the stream.
			/// @param offset The position to seek to.
			/// @param way The direction to seek (beginning, current, end).
			/// @return The new position in the stream.
			std::streamoff seek(std::streampos offset, std::ios_base::seekdir way) override;

			/// @brief Reads data from the stream into a buffer.
			/// @param buffer The buffer to read data into.
			/// @param size The number of bytes to read.
			/// @return The number of bytes actually read.
			size_t read(void* buffer, size_t size) override;

			/// @brief Writes data from a buffer to the stream.
			/// @param buffer The buffer containing data to write.
			/// @param size The number of bytes to write.
			/// @return The number of bytes actually written (default is 0).
			size_t write(const void* buffer, size_t size) override;

			/// @brief Relinquishes ownership of the underlying IGR_Stream.
			/// @return A pointer to the relinquished IGR_Stream.
			IGR_Stream* relinquish_igr_stream() override;

			/// @brief Closes the stream.
			void close();
		protected:
			IGR_Writable_Stream* m_inner = nullptr;
		};

		/// @brief Represents a color with red, green, blue, and alpha components.
		struct Color
		{
			uint8_t r = 0; ///< Red component (0-255).
			uint8_t g = 0; ///< Green component (0-255).
			uint8_t b = 0; ///< Blue component (0-255).
			uint8_t a = 0; ///< Alpha component (0-255), default is 255 (opaque).

			/// @brief Default constructor.
			Color() = default;

			/// @brief Constructs a color with specified red, green, blue, and alpha values.
			/// @param r Red component.
			/// @param g Green component.
			/// @param b Blue component.
			/// @param a Alpha component, default is 255 (opaque).
			Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff)
				: r(r), g(g), b(b), a(a)
			{
			}

			/// @brief Creates a color from an IGR color value.
			/// @param value The IGR color value.
			/// @return The corresponding color_t object.
			static Color from_igr_color(IGR_LONG value)
			{
				Color c;
				c.r = (value >> 16) & 0xFF;
				c.g = (value >> 8) & 0xFF;
				c.b = value & 0xFF;
				c.a = 0xff;
				return c;
			}

			/// @brief Converts the color to an IGR color value.
			/// @return The IGR color value.
			uint32_t to_igr_color() const
			{
				return (a << 24) | (r << 16) | (g << 8) | b;
			}

			/// @brief Equality operator.
			/// @param other The other color to compare with.
			/// @return True if the colors are equal, false otherwise.
			bool operator==(const Color& other) const
			{
				return r == other.r && g == other.g && b == other.b && a == other.a;
			}

			/// @brief Inequality operator.
			/// @param other The other color to compare with.
			/// @return True if the colors are not equal, false otherwise.
			bool operator!=(const Color& other) const
			{
				return !(*this == other);
			}

			/// @brief Returns a black color.
			/// @return A black color_t object.
			static Color black() { return Color{ 0, 0, 0, 0xff }; }

			/// @brief Returns a white color.
			/// @return A white color_t object.
			static Color white() { return Color{ 0xff, 0xff, 0xff, 0xff }; }

			/// @brief Returns a red color.
			/// @return A red color_t object.
			static Color red() { return Color{ 0xff, 0, 0, 0xff }; }

			/// @brief Returns a green color.
			/// @return A green color_t object.
			static Color green() { return Color{ 0, 0xff, 0, 0xff }; }

			/// @brief Returns a blue color.
			/// @return A blue color_t object.
			static Color blue() { return Color{ 0, 0, 0xff, 0xff }; }

			/// @brief Returns a yellow color.
			/// @return A yellow color_t object.
			static Color yellow() { return Color{ 0xff, 0xff, 0, 0xff }; }

			/// @brief Returns a purple color.
			/// @return A purple color_t object.
			static Color purple() { return Color{ 128, 0, 128 }; }

			/// @brief Returns a light coral color.
			/// @return A light coral color_t object.
			static Color light_coral() { return Color{ 240, 128, 128 }; }

			/// @brief Returns a transparent color.
			/// @return A transparent color_t object.
			static Color transparent() { return Color{ 0, 0, 0, 0 }; }

			/// @brief Creates a color from ARGB values.
			/// @param a Alpha component.
			/// @param r Red component.
			/// @param g Green component.
			/// @param b Blue component.
			/// @return The corresponding color_t object.
			static Color from_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) { return Color{ r, g, b, a }; }
		};

		/// @brief Represents an option with various properties.
		class Option
		{
		public:
			/// @brief Constructs an Option object with the given index.
			/// @param index The index of the option.
			Option(uint32_t index);

			/// @brief Gets the display name of the option.
			/// @return The display name as a wide string.
			const std::wstring& getDisplayName() const { return m_display_name; }

			/// @brief Gets the description of the option.
			/// @return The description as a wide string.
			const std::wstring& getDescription() const { return m_description; }

			/// @brief Gets the default value of the option.
			/// @return The default value as a wide string.
			const std::wstring& getDefaultValue() const { return m_default_value; }

			/// @brief Gets the type of the option.
			/// @return The type as a wide string.
			const std::wstring& getType() const { return m_type; }

			/// @brief Gets the flags associated with the option.
			/// @return The flags as a 32-bit unsigned integer.
			uint32_t getFlags() const { return m_flags; }

			/// @brief Gets the possible values for the option.
			/// @return A vector of possible values as wide strings.
			const std::vector<std::wstring>& getPossibleValues() const { return m_possible_values; }

			/// @brief Enum class representing different properties of an option.
			enum class What
			{
				DisplayName = 0, ///< Display name property.
				Description = 1, ///< Description property.
				DefaultValue = 2, ///< Default value property.
				Flags = 3, ///< Flags property.
				Type = 4, ///< Type property.
				PossibleValues = 5 ///< Possible values property.
			};

			/// @brief Fetches a property of an option as a wide string.
			/// @param id The ID of the option.
			/// @param what The property to Fetch.
			/// @param result The fetched property as a wide string.
			/// @return True if the property was fetched successfully, false otherwise.
			static bool Fetch(uint32_t id, What what, std::wstring& result);

			/// @brief Fetches a property of an option as a 32-bit unsigned integer.
			/// @param id The ID of the option.
			/// @param what The property to Fetch.
			/// @param result The fetched property as a 32-bit unsigned integer.
			/// @return True if the property was fetched successfully, false otherwise.
			static bool Fetch(uint32_t id, What what, uint32_t& result);

		private:
			std::wstring m_display_name; ///< The display name of the option.
			std::wstring m_description; ///< The description of the option.
			std::wstring m_default_value; ///< The default value of the option.
			std::wstring m_type; ///< The type of the option.
			uint32_t m_flags = 0; ///< The flags associated with the option.
			std::vector<std::wstring> m_possible_values; ///< The possible values for the option.
		};

		/// @brief Represents a format with various properties.
		class Format
		{
		public:
			/// @brief Constructs a Format object with the given parameters.
			/// @param id The ID of the format.
			/// @param display_name The display name of the format.
			/// @param short_name The short name of the format.
			/// @param config_name The configuration name of the format.
			/// @param mime_type The MIME type of the format.
			/// @param file_type_category The file type category of the format.
			Format(uint32_t id, const std::wstring& display_name, const std::wstring& short_name, const std::wstring& config_name, const std::wstring& mime_type, uint32_t file_type_category)
				: m_id(id), m_display_name(display_name), m_short_name(short_name), m_config_name(config_name), m_mime_type(mime_type), m_file_type_category(file_type_category)
			{
			}

			/// @brief Constructs a Format object with the given ID.
			/// @param id The ID of the format.
			Format(uint32_t id);

			/// @brief Gets the ID of the format.
			/// @return The ID as a 32-bit unsigned integer.
			uint32_t id() const { return m_id; }

			/// @brief Gets the display name of the format.
			/// @return The display name as a wide string.
			const std::wstring& getDisplayName() const { return m_display_name; }

			/// @brief Gets the short name of the format.
			/// @return The short name as a wide string.
			const std::wstring& getShortName() const { return m_short_name; }

			/// @brief Gets the configuration name of the format.
			/// @return The configuration name as a wide string.
			const std::wstring& getConfigName() const { return m_config_name; }

			/// @brief Gets the MIME type of the format.
			/// @return The MIME type as a wide string.
			const std::wstring& getMimeType() const { return m_mime_type; }

			/// @brief Gets the file type category of the format.
			/// @return The file type category as a 32-bit unsigned integer.
			uint32_t getFileTypeCategory() const { return m_file_type_category; }

			/// @brief Enum class representing different properties of a format.
			enum class What
			{
				LongName = 0, ///< Long name property.
				ShortName = 1, ///< Short name property.
				ConfigName = 2, ///< Configuration name property.
				ClassCode = 3, ///< Class code property.
				IsLegacy = 4, ///< Legacy status property.
				MimeType = 5, ///< MIME type property.
				FileCategory = 6 ///< File category property.
			};

			/// @brief Fetches a property of a format as a wide string.
			/// @param id The ID of the format.
			/// @param what The property to fetch.
			/// @param result The fetched property as a wide string.
			/// @return True if the property was fetched successfully, false otherwise.
			static bool Fetch(uint32_t id, What what, std::wstring& result);

			/// @brief Fetches a property of a format as a 32-bit unsigned integer.
			/// @param id The ID of the format.
			/// @param what The property to fetch.
			/// @param result The fetched property as a 32-bit unsigned integer.
			/// @return True if the property was fetched successfully, false otherwise.
			static bool Fetch(uint32_t id, What what, uint32_t& result);

		private:
			uint32_t m_id; ///< The ID of the format.
			std::wstring m_display_name; ///< The display name of the format.
			std::wstring m_short_name; ///< The short name of the format.
			std::wstring m_config_name; ///< The configuration name of the format.
			std::wstring m_mime_type; ///< The MIME type of the format.
			uint32_t m_file_type_category = 0; ///< The file type category of the format.
		};


		/// @brief Enum representing the getType of comparison to perform.
		enum class CompareType
		{
			/// @brief Compare documents at the word level.
			Word = IGR_COMPARE_DOCUMENTS_COMPARE_WORDS
		};

		/// @brief Flags that control the document comparison logic.
		enum class CompareFlags
		{
			/// @brief No comparison getFlags.
			None = 0,

			/// @brief Flag to check for equality.
			Equals = IGR_COMPARE_DOCUMENTS_FLAGS_EQUALS,

			/// @brief Flag to check for moved content.
			Moves = IGR_COMPARE_DOCUMENTS_FLAGS_MOVES,

			/// @brief Flag to check for formatting changes.
			Formatting = IGR_COMPARE_DOCUMENTS_FLAGS_FORMATTING,

			/// @brief Flag to ignore comments.
			NoComments = IGR_COMPARE_DOCUMENTS_FLAGS_NO_COMMENTS,

			/// @brief Flag to ignore case differences.
			NoCase = IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE,

			/// @brief Flag to ignore whitespace differences.
			NoWhitespace = IGR_COMPARE_DOCUMENTS_FLAGS_NO_WHITESPACE,

			/// @brief Flag to ignore punctuation differences.
			NoPunctuation = IGR_COMPARE_DOCUMENTS_FLAGS_NO_PUNCTUATION,

			/// @brief Flag to ignore table differences.
			NoTables = IGR_COMPARE_DOCUMENTS_FLAGS_NO_TABLES,

			/// @brief Flag to ignore header differences.
			NoHeaders = IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS,

			/// @brief Flag to ignore footer differences.
			NoFooters = IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS,

			/// @brief Flag to ignore footnote differences.
			NoFootnotes = IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTNOTES,

			/// @brief Flag to ignore Text box differences.
			NoTextBoxes = IGR_COMPARE_DOCUMENTS_FLAGS_NO_TEXTBOXES,

			/// @brief Flag to ignore field differences.
			NoFields = IGR_COMPARE_DOCUMENTS_FLAGS_NO_FIELDS,
		};
		template<> struct EnableBitMaskOperators<CompareFlags> { static const bool enable = true; };

		/// @enum DifferenceType
		/// @brief Represents the getType of difference found during document comparison.
		enum class DifferenceType
		{
			/// @brief Indicates that the compared sections are equal.
			Equal = IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL,

			/// @brief Indicates that a section was inserted.
			Insert = IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT,

			/// @brief Indicates that a section was deleted.
			Delete = IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE,

			/// @brief Indicates that there is a formatting difference.
			Formatting = IGR_COMPARE_DOCUMENTS_DIFFERENCE_FORMATTING,

			/// @brief Indicates the next batch of differences.
			NextBatch = IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH,
		};

		/// @enum DifferenceSource
		/// @brief Represents the source of the difference found during document comparison.
		enum class DifferenceSource
		{
			/// @brief Indicates that the difference is from the original document.
			Original = 0,

			/// @brief Indicates that the difference is from the revised document.
			Revised = IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_REVISED,

			/// @brief Indicates that the difference is present in both documents.
			Both = IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_BOTH,
		};

		/// @brief Class representing settings for document comparison.
		class CompareSettings
		{
		public:
			/// @brief Constructs a new CompareSettings object.
			CompareSettings();

			/// @brief Gets the getType of comparison to perform.
			/// @return The getType of comparison.
			CompareType getType() const;

			/// @brief Sets the getType of comparison to perform.
			/// @param value The getType of comparison.
			/// @return A reference to the updated CompareSettings object.
			CompareSettings& setType(CompareType value);

			/// @brief Gets the getFlags that control the comparison logic.
			/// @return The comparison getFlags.
			CompareFlags getFlags() const;

			/// @brief Sets the getFlags that control the comparison logic.
			/// @param value The comparison getFlags.
			/// @return A reference to the updated CompareSettings object.
			CompareSettings& setFlags(CompareFlags value);

			/// @brief Sets or unsets a specific comparison flag.
			/// @param value The comparison flag to set or unset.
			/// @param enable True to set the flag, false to unset it.
			/// @return A reference to the updated CompareSettings object.
			CompareSettings& setFlag(CompareFlags value, bool enable = true);

			/// @brief Gets the underlying data structure for Text comparison settings.
			/// @return A reference to the IGR_Text_Compare_Settings structure.
			IGR_Text_Compare_Settings& data() const;
		private:
			std::shared_ptr<IGR_Text_Compare_Settings> m_impl; ///< Implementation details for the comparison settings.
		};

		/// @brief Class representing settings for document comparison.
		class CompareDocumentSettings
		{
		public:
			/// @brief Constructs a new CompareDocumentSettings object.
			CompareDocumentSettings();

			/// @brief Gets the index of the first page to compare.
			/// @return The index of the first page.
			uint32_t getFirstPage() const;

			/// @brief Sets the index of the first page to compare.
			/// @param value The index of the first page.
			/// @return A reference to the updated CompareDocumentSettings object.
			CompareDocumentSettings& setFirstPage(uint32_t value);

			/// @brief Gets the number of pages to compare.
			/// @return The number of pages.
			uint32_t getPageCount() const;

			/// @brief Sets the number of pages to compare.
			/// @param value The number of pages.
			/// @return A reference to the updated CompareDocumentSettings object.
			CompareDocumentSettings& setPageCount(uint32_t value);

			/// @brief Gets the margins to exclude from the comparison.
			/// @return The margins as an RectF structure.
			RectF getMargins() const;

			/// @brief Sets the margins to exclude from the comparison.
			/// @param margins The margins as an RectF structure.
			/// @return A reference to the updated CompareDocumentSettings object.
			CompareDocumentSettings& setMargins(const RectF& margins);

			/// @brief Sets the margins to exclude from the comparison.
			/// @param left The left margin.
			/// @param top The top margin.
			/// @param right The right margin.
			/// @param bottom The bottom margin.
			/// @return A reference to the updated CompareDocumentSettings object.
			CompareDocumentSettings& setMargins(float left, float top, float right, float bottom);

		protected:
			/// @brief Implementation details for the document comparison settings.
			std::shared_ptr<IGR_Text_Compare_Document_Source> m_impl;
		};

		/// @class CompareDocumentSource
		/// @brief Represents a source document for comparison.
		/// 
		/// This class is used to define a source document that will be compared against another document.
		/// It inherits from CompareDocumentSettings to utilize common comparison settings.
		class CompareDocumentSource : public CompareDocumentSettings
		{
		public:
			/// @brief Default constructor for CompareDocumentSource.
			/// 
			/// Initializes a new instance of the CompareDocumentSource class.
			CompareDocumentSource() = default;

			/// @brief Constructor for CompareDocumentSource with a document handle.
			/// 
			/// @param handle The handle to an open document.
			CompareDocumentSource(IGR_HDOC handle, const CompareDocumentSettings& settings = CompareDocumentSettings());

			/// @brief Constructor for CompareDocumentSource with an Extractor.
			/// 
			/// @param Extractor The Extractor object used to initialize the CompareDocumentSource.
			CompareDocumentSource(const Extractor& Extractor, const CompareDocumentSettings& settings = CompareDocumentSettings());

			/// @brief Assigns the specified comparison settings to the CompareDocumentSource.
			/// @param settings The settings to assign to the CompareDocumentSource.
			/// @return A reference to the updated CompareDocumentSource.
			CompareDocumentSource& assign(const CompareDocumentSettings& settings);

			/// @brief Retrieves the handle to the document.
			/// @return The handle to the document as an IGR_HDOC.
			IGR_HDOC getHandle() const;

			/// @brief Retrieves the Text comparison document source data.
			/// @return A constant reference to the IGR_Text_Compare_Document_Source data.
			const IGR_Text_Compare_Document_Source& data() const;
		};

		/// @class CompareResultDifferenceDetail
		/// @brief Represents the details of a difference found during document comparison.
		class CompareResultDifferenceDetail
		{
		public:
			/// @brief Default constructor for CompareResultDifferenceDetail.
			/// 
			/// Initializes a new instance of the CompareResultDifferenceDetail class.
			CompareResultDifferenceDetail();

			/// @brief Constructor for CompareResultDifferenceDetail with a reference item.
			/// 
			/// @param ref The reference item containing the difference details.
			CompareResultDifferenceDetail(const IGR_Compare_Documents_Difference_Item& ref);

			/// @brief Checks if the difference detail is valid.
			bool ok() const;

			/// @brief Gets the page index where the difference was found.
			/// 
			/// @return The page index.
			uint32_t getPageIndex() const;

			/// @brief Gets the bounds of the difference.
			/// 
			/// @return The bounding rectangle of the difference.
			RectF getBounds() const;

			/// @brief Gets the Text associated with the difference.
			/// 
			/// @return The Text of the difference.
			std::wstring getText() const;
		private:
			/// @brief Implementation details for CompareResultDifferenceDetail.
			class impl_t;

			/// @brief Shared pointer to the implementation details.
			std::shared_ptr<impl_t> m_impl;
		};

		/// @class CompareResultDifference
		/// @brief Represents a difference found during document comparison.
		class CompareResultDifference
		{
		public:
			/// @brief Type definition for a lazy-loaded list of CompareResultDifferenceDetail objects.
			typedef lazy_loader_indexed<CompareResultDifferenceDetail> details_t;

			/// @brief Type definition for a constant iterator over the details list.
			typedef details_t::const_iterator const_iterator;

			/// @brief Default constructor for CompareResultDifference.
			/// 
			/// Initializes a new instance of the CompareResultDifference class.
			CompareResultDifference();

			/// @brief Constructor for CompareResultDifference with a reference difference.
			/// 
			/// @param ref The reference difference containing the details.
			CompareResultDifference(const IGR_Compare_Documents_Difference& ref);

			/// @brief Checks if the difference is valid.
			bool ok() const;

			/// @brief Gets the getType of the difference.
			/// 
			/// @return The getType of the difference.
			DifferenceType getType() const;

			/// @brief Gets the source of the difference.
			/// 
			/// @return The source of the difference.
			DifferenceSource getSource() const;

			/// @brief Gets the page index in the original document where the difference was found.
			/// 
			/// @return The page index in the original document.
			uint32_t getOriginalPageIndex() const;

			/// @brief Gets the page index in the revised document where the difference was found.
			/// 
			/// @return The page index in the revised document.
			uint32_t getRevisedPageIndex() const;

			/// @brief Gets the Text associated with the difference.
			/// 
			/// @return The Text of the difference.
			std::wstring getText() const;

			/// @brief Gets the details of the difference.
			/// 
			/// @return A reference to the details list.
			details_t& details() const;

			/// @brief Gets the number of details in the difference.
			/// 
			/// @return The number of details.
			size_t size() const;

			/// @brief Accesses a specific detail by index.
			/// 
			/// @param index The index of the detail.
			/// @return The detail at the specified index.
			CompareResultDifferenceDetail operator[](size_t index) const;

			/// @brief Accesses a specific detail by index with bounds checking.
			/// 
			/// @param index The index of the detail.
			/// @return The detail at the specified index.
			CompareResultDifferenceDetail at(size_t index) const;

			/// @brief Gets an iterator to the beginning of the details list.
			/// 
			/// @return An iterator to the beginning of the details list.
			const_iterator begin() const;

			/// @brief Gets an iterator to the end of the details list.
			/// 
			/// @return An iterator to the end of the details list.
			const_iterator end() const;
		private:
			/// @brief Implementation details for CompareResultDifference.
			class impl_t;

			/// @brief Shared pointer to the implementation details.
			std::shared_ptr<impl_t> m_impl;
		};

		/// @class CompareResults
		/// @brief Manages the results of a document comparison operation.
		class CompareResults
		{
		public:
			/// @brief Constructor for CompareResults.
			/// 
			/// Initializes a new instance of the CompareResults class with an optional handle.
			/// @param handle The handle to the Text comparison results. Defaults to 0.
			CompareResults(IGR_HTEXTCOMPARE handle = 0);

			/// @brief Closes the comparison results.
			void Close();

			/// @brief Resets the comparison results to the initial state.
			void Reset();

			/// @brief Moves to the next difference in the comparison results.
			/// @return True if there is a next difference, false otherwise.
			bool MoveNext();

			/// @brief Gets the current difference in the comparison results.
			/// @return The current CompareResultDifference.
			CompareResultDifference getCurrent() const;

			/// @brief Gets the next difference in the comparison results.
			/// @return The next CompareResultDifference.
			CompareResultDifference getNext();
		private:
			/// @brief Implementation details for CompareResults.
			class impl_t;

			/// @brief Shared pointer to the implementation details.
			std::shared_ptr<impl_t> m_impl;
		};

		/// @brief The `Extractor` class provides functionality for handling and processing documents, including opening, saving, copying, retrieving metadata, and managing callbacks for various events.
		class Extractor
		{
		public:
			typedef std::function<std::wstring(const std::wstring&)> password_callback_t;
			typedef std::function<std::wstring(IGR_ULONG, const std::wstring&)> localize_callback_t;
			typedef std::function<int()> heartbeat_callback_t;
			typedef std::function<IGR_LOG_LEVEL_TYPE(const std::string&)> log_level_callback_t;
			typedef std::function<void(IGR_LOG_LEVEL_TYPE, const std::string&, const std::string&)> log_message_callback_t;
			typedef std::function<bool(const std::wstring&)> approve_external_resource_callback_t;
			typedef std::function<std::unique_ptr<Stream>(const std::wstring&)> get_resource_stream_callback_t;
			typedef std::function<bool(OcrImage&)> ocr_image_callback_t;
			typedef lazy_loader_indexed<Page> pages_t;
			typedef enumerable_t<Subfile> subfiles_t;


			Extractor();
			Extractor(IGR_Stream* stream);
			virtual ~Extractor() {};

			IGR_HDOC getHandle() const;

			/// @brief Closes the document.
			void Close();

			/// Opens a document with the specified mode and options.
			///
			/// @param mode The open mode for the document.
			/// @param open_flags The open getFlags for the document. Default value is IGR_BODY_AND_META.
			/// @param option The additional options for opening the document. Default value is an empty string.
			/// @param callback The callback function to be called after the document is opened. Default value is an empty function.
			void Open(OpenMode mode, uint32_t open_flags = IGR_BODY_AND_META, const std::wstring& option = std::wstring(), const DocumentFilters::open_callback_t& callback = nullptr);

			/// Opens a document with the specified open getFlags, option, and callback.
			///
			/// @param open_flags The getFlags indicating which parts of the document to open. Default is IGR_BODY_AND_META.
			/// @param option The option for opening the document. Default is an empty string.
			/// @param callback The callback function to be called after the document is opened. Default is an empty callback.
			void Open(uint32_t open_flags = IGR_BODY_AND_META, const std::wstring& option = std::wstring(), const DocumentFilters::open_callback_t& callback = nullptr);

			/// @brief Retrieves the file getType.
			///
			/// @return The file getType as a 32-bit unsigned integer.
			uint32_t getFileType() const;

			/// @brief Retrieves the file capabilities.
			///
			/// @return The file capabilities as a 32-bit unsigned integer.
			uint32_t getFileCapabilities() const;

			/// @brief Retrieves the file getType as a wide string.
			///
			/// @param what The getType of file getType to retrieve.
			/// @return The file getType as a wide string.
			std::wstring getFileType(Format::What what) const;

			/// @brief Retrieves the MIME getType of the document.
			/// @details This function returns the MIME getType of the document by calling the get_file_type function with the MimeType parameter.
			/// @return A std::wstring containing the MIME getType of the document.
			std::wstring getMimeType() const { return getFileType(Format::What::MimeType); }

			/// @brief Retrieves if the file supports a specific capability.
			///
			/// @return The capabilities state.
			bool getSupportsText() { return getFileCapabilities() & IGR_FILE_SUPPORTS_TEXT; }

			/// @brief Retrieves if the file supports a specific capability.
			///
			/// @return The capabilities state.
			bool getSupportsSubFiles() { return getFileCapabilities() & IGR_FILE_SUPPORTS_SUBFILES; }

			/// @brief Retrieves if the file supports a specific capability.
			///
			/// @return The capabilities state.
			bool getSupportsHtml() { return getFileCapabilities() & IGR_FILE_SUPPORTS_HDHTML; }

			/**
			 * @brief Retrieves the next block of Text from the file.
			 *
			 * @param max_length The maximum length of the block to retrieve.
			 * @param strip_control_chars Flag indicating whether to strip control characters from the Text.
			 * @return The next block of Text as a wide string.
			 */
			std::wstring getText(size_t max_length, bool strip_control_chars = false);

			/// @brief Returns the end-of-file status of the object.
			///
			/// @return true if the object has reached the end of the file, false otherwise.
			bool getEOF() const;

			/// Saves the Text of the document to a file.
			///
			/// @param filename The path of the file to save the object to.
			/// @param code_page The code page to use for saving the file. Default is 65001.
			void SaveTo(const std::wstring& filename, int code_page = 65001);

			/// Saves the Text of the document to a stream.
			///
			/// @param stream The stream to save the object to.
			/// @param code_page The code page to use for saving the file. Default is 65001.
			void SaveTo(std::ostream& Stream, int code_page = 65001);

			/// Copies the bytes of the document to a file.
			///
			/// @param filename The path of the file to copy the object to.
			void CopyTo(const std::wstring& filename);

			/// Copies the bytes of the document to a stream.
			///
			/// @param stream The stream to copy the object to.
			void CopyTo(std::ostream& Stream);

			/// Copies the bytes of the document to a stream.
			///
			/// @param stream The stream to copy the object to.
			void CopyTo(Stream& Stream);

			/// @brief Retrieves the MD5 hash of the current object.
			///
			/// @return The MD5 hash as a wide string.

			std::wstring getHashMD5();
			/// @brief Retrieves the SHA1 hash of the document.
			///
			/// @return The SHA1 hash of the document as a wide string.
			std::wstring getHashSHA1();

			/// @brief Returns the number of pages in the document.
			///
			/// @return The number of pages in the document.
			size_t getPageCount() const;

			/// Retrieves the page at the specified index.
			///
			/// @param index The index of the page to retrieve.
			/// @return The page at the specified index.
			Page getPage(size_t index) const;

			/// @brief Returns the pages of the document.
			///
			/// @return A constant reference to the pages of the document.
			const pages_t& pages() const;

			/// Returns a constant reference to the subfiles.
			///
			/// @return A constant reference to the subfiles.
			const subfiles_t& subfiles() const;

			/// Retrieves the subfile with the specified ID.
			///
			/// @param id The ID of the subfile to retrieve.
			/// @return The subfile object with the specified ID.
			Subfile getSubFile(const std::wstring& id) const;

			/// Returns a constant reference to the collection of subfiles representing images.
			///
			/// @return A constant reference to the collection of subfiles representing images.
			const subfiles_t& images() const;

			/// Retrieves the image with the specified ID.
			///
			/// @param id The ID of the image to retrieve.
			/// @return The subfile object representing the image.
			Subfile getImage(const std::wstring& id) const;

			/// Sets the password callback function.
			///
			/// @param callback The callback function to be set.
			void setPasswordCallback(const password_callback_t& callback);

			/// Sets the callback function for localization.
			///
			/// @param callback The callback function to be set.
			void setLocalizeCallback(const localize_callback_t& callback);

			/// Sets the callback function for the heartbeat event.
			///
			/// @param callback The callback function to be called when a heartbeat event occurs.
			void setHeartbeatCallback(const heartbeat_callback_t& callback);

			/// Sets the callback function for the log level.
			///
			/// @param callback The callback function to be called when the log level is set.
			void setLogLevelCallback(const log_level_callback_t& callback);

			/// Sets the callback function for log messages.
			/// @param callback The callback function to be called when a log message is generated.
			void setLogMessageCallback(const log_message_callback_t& callback);

			/// Sets the callback function for approving external resources.
			/// 
			/// @param callback The callback function to be called when an external resource is requested.
			void setApproveExternalResourceCallback(const approve_external_resource_callback_t& callback);

			/// Sets the callback function for getting a resource stream.
			/// 
			/// @param callback The callback function to be called when a resource stream is requested.
			void setGetResourceStreamCallback(const get_resource_stream_callback_t& callback);

			/// @brief Sets the callback function for OCR image processing.
			/// 
			/// @param callback The callback function to be set for OCR image processing.
			void setOcrImageCallback(const ocr_image_callback_t& callback);

			/// Retrieves the bookmarks in the document.
			///
			/// @return The bookmarks in the document.
			Bookmark getRootBookmark() const;

			/// @brief Steals the stream and returns a pointer to the IGR_Stream.
			///
			/// This function transfers ownership of the stream to the caller.
			///
			/// @return A pointer to the IGR_Stream.
			IGR_Stream* StealStream();

			/// @brief Compares the current document with another document using the specified settings.
			/// @param other The other document to compare with.
			/// @param settings The settings to use for the comparison. Defaults to CompareSettings().
			/// @return The results of the comparison.
			CompareResults Compare(const Extractor& other, const CompareSettings& settings = CompareSettings()) const;

			/// @brief Compares the current document with another document using the specified document settings and comparison settings.
			/// @param thisDocSettings The settings for the current document.
			/// @param other The other document to compare with.
			/// @param otherDocSettings The settings for the other document.
			/// @param settings The settings to use for the comparison. Defaults to CompareSettings().
			/// @return The results of the comparison.
			CompareResults Compare(const CompareDocumentSettings& thisDocSettings, const Extractor& other, const CompareDocumentSettings& otherDocSettings, const CompareSettings& settings = CompareSettings()) const;

			/// @brief Compares the current document with another document source using the specified document settings and comparison settings.
			/// @param thisDocSettings The settings for the current document.
			/// @param other The other document source to compare with.
			/// @param settings The settings to use for the comparison. Defaults to CompareSettings().
			/// @return The results of the comparison.
			CompareResults Compare(const CompareDocumentSettings& thisDocSettings, const CompareDocumentSource& other, const CompareSettings& settings = CompareSettings()) const;

			/// @brief Compares two document sources using the specified comparison settings.
			/// @param thisDoc The first document source to compare.
			/// @param otherDoc The second document source to compare.
			/// @param settings The settings to use for the comparison. Defaults to CompareSettings().
			/// @return The results of the comparison.
			static CompareResults Compare(const CompareDocumentSource& thisDoc, const CompareDocumentSource& otherDoc, const CompareSettings& settings = CompareSettings());

		protected:
			virtual IGR_Stream* resolve_stream() const;
			IGR_Stream* need_stream() const;

		protected:
			class impl_t;
			std::shared_ptr<impl_t> m_impl;
		};

		/// @brief Represents a subfile within a document, providing access to its properties and data streams.
		class Subfile : public Extractor
		{
			friend class Extractor;
			friend class Page;

		protected:
			/// @brief Type definition for a function that opens a stream for a subfile.
			typedef std::function<IGR_RETURN_CODE(IGR_LONG doc, const IGR_UCS2* id, struct IGR_Stream** Stream, Error_Control_Block* Error)> stream_opener_t;

			/// @brief Resolves and returns the stream associated with the subfile.
			/// @return A pointer to the resolved IGR_Stream.
			IGR_Stream* resolve_stream() const override;

		public:
			/// @brief Default constructor for the Subfile class.
			Subfile();

			/// @brief Constructs a Subfile with the specified document handle, subfile info, and stream opener.
			/// @param doc_handle The handle to the document containing the subfile.
			/// @param info The information about the subfile.
			/// @param stream_opener The function to open the stream for the subfile.
			Subfile(IGR_LONG doc_handle, const IGR_Subfile_Info& info, const stream_opener_t& stream_opener);

			/// @brief Constructs a Subfile with the specified document handle, subfile ID, and stream.
			/// @param doc_handle The handle to the document containing the subfile.
			/// @param id The ID of the subfile.
			/// @param stream The stream associated with the subfile.
			Subfile(IGR_LONG doc_handle, const std::wstring& id, IGR_Stream* Stream);

			/// @brief Retrieves the ID of the subfile.
			/// @return The ID of the subfile as a wide string.
			std::wstring getId() const;

			/// @brief Retrieves the name of the subfile.
			/// @return The name of the subfile as a wide string.
			std::wstring getName() const;

			/// @brief Retrieves the size of the subfile.
			/// @return The size of the subfile in bytes.
			uint64_t getSize() const;

			/// @brief Retrieves the getFlags associated with the subfile.
			/// @return The getFlags of the subfile as a 32-bit unsigned integer.
			uint32_t getFlags() const;

			/// @brief Checks if the subfile is encrypted.
			/// @return True if the subfile is encrypted, otherwise false.
			bool getIsEncrypted() const;

			/// @brief Retrieves the comment associated with the subfile.
			/// @return The comment of the subfile as a wide string.
			std::wstring getComment() const;

			/// @brief Retrieves the file date of the subfile.
			/// @return The file date of the subfile as a DateTime object.
			DateTime getFileDate() const;

			/// @brief Equality operator to compare two Subfile objects based on their IDs.
			/// @param other The other Subfile object to compare with.
			/// @return True if the IDs of both Subfile objects are equal, otherwise false.
			bool operator==(const Subfile& other) const { return getId() == other.getId(); }

		private:
			IGR_LONG m_owning_doc = 0; ///< The handle to the owning document.
			stream_opener_t m_stream_opener = nullptr; ///< The function to open the stream for the subfile.
			std::wstring m_id; ///< The ID of the subfile.
			std::wstring m_name; ///< The name of the subfile.
			uint64_t m_size = 0; ///< The size of the subfile in bytes.
			uint32_t m_flags = 0; ///< The getFlags associated with the subfile.
			std::wstring m_comment; ///< The comment associated with the subfile.
			DateTime m_file_date; ///< The file date of the subfile.
		};


		class Page
		{
			friend class Extractor;

		protected:
			Page(IGR_HPAGE page_handle, size_t index);

		public:
			typedef lazy_loader_indexed<Word> words_t;
			typedef Extractor::subfiles_t images_t;
			typedef lazy_loader_indexed<std::shared_ptr<Annotation>> annotations_t;

			Page();

			/// @brief Returns the index of the object.
			///
			/// @return The index of the object.
			size_t getIndex() const;

			/// @brief Returns the width of the object.
			///
			/// @return The width of the object as a uint32_t value.
			uint32_t getWidth() const;

			/// @brief Retrieves the height of the object.
			///
			/// @return The height of the object as a uint32_t value.
			uint32_t getHeight() const;

			/// @brief Returns the number of words in the document.
			///
			/// @return The number of words in the document.
			size_t getWordCount() const;

			/// @brief Retrieves the size of the object.
			/// @return An IGR_Size structure containing the width and height of the object.
			IGR_Size getSize() const { return IGR_Size{ getWidth(), getHeight() }; }

			/// Retrieves the Text associated with the object.
			///
			/// @return The Text as a wide string.
			std::wstring getText();

			/// @brief Retrieves the handle of the IGR_HPAGE object.
			///
			/// @return The handle of the IGR_HPAGE object.
			IGR_HPAGE getHandle() const;

			/// Retrieves the word at the specified index.
			///
			/// @param index The index of the word to retrieve.
			/// @return The word at the specified index.
			Word getWord(size_t index) const;

			/// @brief Get the reference to the words in the document.
			///
			/// @return const reference to the words in the document.
			const words_t& words() const;

			/// @brief Get the root page element of the page.
			///
			/// @return The root page element of the page.
			PageElement getRootPageElement() const;

			/// @brief Retrieves a collection of images.
			/// @return A constant reference to a collection of images.
			const images_t& images() const;

			/// @brief Retrieves a specific image by its identifier.
			/// @param id The identifier of the image to retrieve.
			/// @return A constant subfile object representing the image.
			Subfile getImage(const std::wstring& id) const;

			/// @brief Retrieves a collection of form elements.
			/// @return A constant reference to a vector of form elements.
			const std::vector<FormElement>& formElements() const;

			/// @brief Retrieves a collection of hyperlinks.
			/// @return A constant reference to a vector of hyperlinks.
			const std::vector<Hyperlink>& hyperlinks() const;

			/// @brief Retrieves the list of annotations.
			///
			/// @return A constant reference to a vector containing annotation objects.
			const annotations_t& annotations() const;

			/// @brief Renders the content onto the provided canvas.
			///
			/// @param canvas The canvas to render the content onto.
			void Render(Canvas& Canvas) const;

			/// @brief Renders the content onto the provided canvas with specific properties.
			///
			/// @param canvas The canvas to render the content onto.
			/// @param properties The properties to use for rendering the page.
			void Render(Canvas& Canvas, const RenderPageProperties& properties) const;

			/// @brief Renders the content onto the provided canvas with specific options and properties.
			///
			/// @param canvas The canvas to render the content onto.
			/// @param options The options to use for rendering the page.
			/// @param properties The properties to use for rendering the page.
			void Render(Canvas& Canvas, std::wstring& options, const RenderPageProperties& properties) const;

			/// @brief Retrieves the value of a specified attribute.
			///
			/// @param name The name of the attribute to retrieve.
			/// @return A std::wstring containing the value of the specified attribute.
			std::wstring getAttribute(const std::wstring& name) const;

			/// @brief Gets the pixels of the page.
			/// @return The pixels of the page.
			PagePixels getPixels() const;

			/// @brief Gets the pixels of the page with a specified pixel getType.
			/// @param getType The getType of pixels to retrieve.
			/// @return The pixels of the page.
			PagePixels getPixels(PixelType type) const;

			/// @brief Gets the pixels of the page with a specified pixel getType and source rectangle.
			/// @param getType The getType of pixels to retrieve.
			/// @param src_rect The source rectangle to extract pixels from.
			/// @param options Optional parameters for pixel extraction.
			/// @return The pixels of the page.
			PagePixels getPixels(PixelType type, const IGR_Rect& src_rect, const std::wstring& options = std::wstring()) const;

			/// @brief Gets the pixels of the page with a specified pixel getType, source rectangle, and destination size.
			/// @param getType The getType of pixels to retrieve.
			/// @param src_rect The source rectangle to extract pixels from.
			/// @param dest_size The destination size for the extracted pixels.
			/// @param options Optional parameters for pixel extraction.
			/// @return The pixels of the page.
			PagePixels getPixels(PixelType type, const IGR_Rect& src_rect, const IGR_Size& dest_size, const std::wstring& options = std::wstring()) const;

			/// @brief Compares the current page with another page using the specified settings.
			/// @param other The other page to compare with.
			/// @param settings The settings to use for the comparison. Defaults to CompareSettings().
			/// @return The results of the comparison.
			CompareResults Compare(const Page& other, const CompareSettings& settings = CompareSettings()) const;

			/// @brief Compares the current page with another page using the specified margins and settings.
			/// @param other The other page to compare with.
			/// @param margins The margins to use for the comparison.
			/// @param settings The settings to use for the comparison. Defaults to CompareSettings().
			/// @return The results of the comparison.
			CompareResults Compare(const Page& other, const RectF& margins, const CompareSettings& settings = CompareSettings()) const;

			/// @brief Compares the current page with another page using the specified left and right margins and settings.
			/// @param other The other page to compare with.
			/// @param leftMargins The left margins to use for the comparison.
			/// @param rightMargins The right margins to use for the comparison.
			/// @param settings The settings to use for the comparison. Defaults to CompareSettings().
			/// @return The results of the comparison.
			CompareResults Compare(const Page& other, const RectF& leftMargins, const RectF& rightMargins, const CompareSettings& settings = CompareSettings()) const;

		private:
			class impl_t;
			std::shared_ptr<impl_t> m_impl;
		};

		class Word
		{
			friend class Page;

		protected:
			Word(const IGR_Page_Word& Word, size_t index);

		public:
			/// @brief Retrieves the Text content of the word.
			/// @return The Text content of the word as a wide string.
			std::wstring getText() const;

			/// @brief Retrieves the x-coordinate of the word's position.
			/// @return The x-coordinate of the word's position.
			int32_t getX() const;

			/// @brief Retrieves the y-coordinate of the word's position.
			/// @return The y-coordinate of the word's position.
			int32_t getY() const;

			/// @brief Retrieves the width of the word.
			/// @return The width of the word.
			int32_t getWidth() const;

			/// @brief Retrieves the height of the word.
			/// @return The height of the word.
			int32_t getHeight() const;

			/// @brief Retrieves the rightmost x-coordinate of the word's bounding box.
			/// @return The rightmost x-coordinate of the word's bounding box.
			int32_t getRight() const;

			/// @brief Retrieves the bottom y-coordinate of the word's bounding box.
			/// @return The bottom y-coordinate of the word's bounding box.
			int32_t getBottom() const;

			/// @brief Retrieves the bounding box of the word.
			RectI32 getRect() const;

			/// @brief Retrieves the character offset of the word within the page.
			/// @return The character offset of the word within the page.
			size_t getCharacterOffset() const;

			/// @brief Retrieves the index of the word within the page.
			/// @return The index of the word within the page.
			size_t getWordIndex() const;

			// @brief Retrieves the IGR_Page_Word of the word.
			const IGR_Page_Word* data() const;

		private:
			IGR_Page_Word m_word;
			size_t m_index;
		};

		enum class RenderPagePropertiesFlags : uint32_t
		{
			DoNotRedactText = IGR_REDACT_FLAGS_DO_NOT_REDACT_TEXT,
			DoNotRedactImages = IGR_REDACT_FLAGS_DO_NOT_REDACT_IMAGES,
			AlwaysRaster = IGR_REDACT_FLAGS_ALWAYS_RASTER,
			NeverRaster = IGR_REDACT_FLAGS_NEVER_RASTER,
			DoNotDrawRegion = IGR_REDACT_FLAGS_DO_NOT_DRAW_REGION
		};
		template<> struct EnableBitMaskOperators<RenderPagePropertiesFlags> { static const bool enable = true; };

		/// @brief Represents properties for rendering a page, including source and destination rectangles, getFlags, redactions, and form values.
		class RenderPageProperties
		{
		public:
			/// @brief Type definition for a form value tuple consisting of name, value, and selection status.
			typedef std::tuple<std::wstring, std::wstring, bool> form_value_t;

			/// @brief Type definition for a vector of form values.
			typedef std::vector<form_value_t> form_values_t;

			/// @brief Default constructor for RenderPageProperties.
			RenderPageProperties();

			/// @brief Retrieves the underlying data structure for rendering page properties.
			/// @return A pointer to the IGR_Render_Page_Properties structure.
			IGR_Render_Page_Properties* data() const;

			/// @brief Sets the source rectangle for rendering.
			/// @param value The source rectangle to set.
			/// @return A reference to the updated RenderPageProperties object.
			RenderPageProperties& setSourceRect(const IGR_Rect& value);

			/// @brief Sets the source rectangle for rendering using individual coordinates.
			/// @param left The left coordinate.
			/// @param top The top coordinate.
			/// @param right The right coordinate.
			/// @param bottom The bottom coordinate.
			/// @return A reference to the updated RenderPageProperties object.
			RenderPageProperties& setSourceRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

			/// @brief Sets the destination rectangle for rendering.
			/// @param value The destination rectangle to set.
			/// @return A reference to the updated RenderPageProperties object.
			RenderPageProperties& setDestRect(const IGR_Rect& value);

			/// @brief Sets the destination rectangle for rendering using individual coordinates.
			/// @param left The left coordinate.
			/// @param top The top coordinate.
			/// @param right The right coordinate.
			/// @param bottom The bottom coordinate.
			/// @return A reference to the updated RenderPageProperties object.
			RenderPageProperties& setDestRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

			/// @brief Retrieves the getFlags associated with rendering properties.
			/// @return The getFlags as a 32-bit unsigned integer.
			uint32_t getFlags() const;

			/// @brief Sets the getFlags associated with rendering properties.
			/// @param value The getFlags to set.
			/// @return A reference to the updated RenderPageProperties object.
			RenderPageProperties& setFlags(uint32_t value);

			/// @brief Sets a specific flag for rendering properties.
			/// @param flag The flag to set.
			/// @param val The value to set for the flag.
			/// @return A reference to the updated RenderPageProperties object.
			RenderPageProperties& setFlag(RenderPagePropertiesFlags flag, bool val);

			/// @brief Adds a redaction to the rendering properties.
			/// @param rect The rectangle defining the redaction area.
			/// @param c The color of the redaction. Defaults to black.
			/// @return A reference to the updated RenderPageProperties object.
			RenderPageProperties& AddRedaction(const RectI32& rect, const Color& c = Color::black());

			/// @brief Adds a redaction to the rendering properties using individual coordinates.
			/// @param left The left coordinate.
			/// @param top The top coordinate.
			/// @param right The right coordinate.
			/// @param bottom The bottom coordinate.
			/// @param c The color of the redaction. Defaults to black.
			/// @return A reference to the updated RenderPageProperties object.
			RenderPageProperties& AddRedaction(int32_t left, int32_t top, int32_t right, int32_t bottom, const Color& c = Color::black());

			/// @brief Adds a form value to the rendering properties.
			/// @param name The name of the form field.
			/// @param value The value of the form field.
			/// @param selected The selection status of the form field.
			/// @return A reference to the updated RenderPageProperties object.
			RenderPageProperties& AddFormValue(const std::wstring& name, const std::wstring& value, bool selected);

			/// @brief Adds a form value to the rendering properties.
			/// @param value The form value tuple to add.
			/// @return A reference to the updated RenderPageProperties object.
			RenderPageProperties& AddFormValue(const form_value_t& value);

			/// @brief Retrieves the form values associated with the rendering properties.
			/// @return A constant reference to the vector of form values.
			const form_values_t& form_values() const;

			/// @brief Retrieves the count of form values.
			/// @return The number of form values.
			size_t getFormValueCount() const;

			/// @brief Retrieves a specific form value by index.
			/// @param index The index of the form value to retrieve.
			/// @return A constant reference to the form value tuple.
			const form_value_t& getFormValue(size_t index) const;

			/// @brief Checks if the rendering properties are empty.
			/// @return True if the rendering properties are empty, otherwise false.
			bool empty() const;

		private:
			/// @brief Implementation details for RenderPageProperties.
			class impl_t;

			/// @brief Shared pointer to the implementation details.
			std::shared_ptr<impl_t> m_impl;
		};

		enum class PenStyle
		{
			None = IGR_PEN_NONE,
			Solid = IGR_PEN_SOLID,
		};

		enum class BrushStyle
		{
			None = IGR_BRUSH_NONE,
			Solid = IGR_BRUSH_SOLID,
		};

		enum class FontStyle : uint32_t
		{
			Normal = 0,
			Bold = IGR_TEXT_STYLE_BOLD,
			Italic = IGR_TEXT_STYLE_ITALIC,
			Underline = IGR_TEXT_STYLE_UNDERLINE,
		};
		template<> struct EnableBitMaskOperators<FontStyle> { static const bool enable = true; };

		class AnnotationSerializable
		{
		public:
			virtual ~AnnotationSerializable() = default;
			virtual std::string serialize() const = 0;
			virtual void bind(const IGR_Annotation& annot) {}
			bool operator==(const AnnotationSerializable& other) const { return true; }
		};

		/// @brief Represents a canvas for rendering pages.
		class Canvas
		{
			friend class DocumentFilters;
		protected:
			/// @brief Constructs a Canvas with a given handle.
			/// @param handle The handle to the canvas.
			/// @param stream The writable stream associated with the canvas.
			/// @param own_stream Indicates whether the canvas owns the stream.
			Canvas(IGR_HCANVAS handle, IGR_Writable_Stream* stream, bool own_stream);

		public:
			/// @brief Default constructor for Canvas.
			Canvas();

			/// @brief Gets the handle of the canvas.
			/// @return The handle of the canvas.
			IGR_HCANVAS getHandle() const;

			/// @brief Indicates if the canvas has a handle.
			/// @return True if the canvas has a handle, otherwise false.
			bool hasHandle() const;

			/// @brief Closes the canvas.
			void Close();

			/// @brief Renders a page onto the canvas.
			/// @param page The page to render.
			/// @param options Optional rendering options.
			/// @param properties Optional rendering properties.
			void RenderPage(const Page& Page, const std::wstring& options = std::wstring(), const RenderPageProperties& properties = RenderPageProperties());

			/// @brief Renders a page onto the canvas with specified properties.
			/// @param page The page to render.
			/// @param properties The rendering properties.
			void RenderPage(const Page& Page, const RenderPageProperties& properties);

			/// @brief Renders multiple pages onto the canvas.
			/// @param Extractor The extractor containing the pages to render.
			void RenderPages(const Extractor& Extractor);

			/// @brief Creates a blank page on the canvas.
			/// @param width The width of the blank page.
			/// @param height The height of the blank page.
			/// @param options Optional rendering options.
			void BlankPage(int width, int height, const std::wstring& options = std::wstring());
			/// @brief Draws an arc defined by four points.
			/// @param x The x-coordinate of the starting point.
			/// @param y The y-coordinate of the starting point.
			/// @param x2 The x-coordinate of the ending point.
			/// @param y2 The y-coordinate of the ending point.
			/// @param x3 The x-coordinate of the control point 1.
			/// @param y3 The y-coordinate of the control point 1.
			/// @param x4 The x-coordinate of the control point 2.
			/// @param y4 The y-coordinate of the control point 2.
			void Arc(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4);

			/// @brief Draws an angled arc.
			/// @param x The x-coordinate of the center.
			/// @param y The y-coordinate of the center.
			/// @param radius The radius of the arc.
			/// @param start_angle The starting angle of the arc.
			/// @param sweep_angle The sweep angle of the arc.
			void AngleArc(int x, int y, int radius, double start_angle, double sweep_angle);

			/// @brief Draws a chord.
			/// @param x The x-coordinate of the bounding rectangle's top-left corner.
			/// @param y The y-coordinate of the bounding rectangle's top-left corner.
			/// @param x2 The x-coordinate of the bounding rectangle's bottom-right corner.
			/// @param y2 The y-coordinate of the bounding rectangle's bottom-right corner.
			/// @param x3 The x-coordinate of the starting point of the chord.
			/// @param y3 The y-coordinate of the starting point of the chord.
			/// @param x4 The x-coordinate of the ending point of the chord.
			/// @param y4 The y-coordinate of the ending point of the chord.
			void Chord(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4);

			/// @brief Draws an ellipse.
			/// @param x The x-coordinate of the bounding rectangle's top-left corner.
			/// @param y The y-coordinate of the bounding rectangle's top-left corner.
			/// @param x2 The x-coordinate of the bounding rectangle's bottom-right corner.
			/// @param y2 The y-coordinate of the bounding rectangle's bottom-right corner.
			void Ellipse(int x, int y, int x2, int y2);

			/// @brief Draws an ellipse within a specified rectangle.
			/// @param rect The rectangle defining the bounds of the ellipse.
			void Ellipse(const RectI32& rect) { Ellipse(rect.left, rect.top, rect.right, rect.bottom); }

			/// @brief Draws a line between two points.
			/// @param x The x-coordinate of the starting point.
			/// @param y The y-coordinate of the starting point.
			/// @param x2 The x-coordinate of the ending point.
			/// @param y2 The y-coordinate of the ending point.
			void Line(int x, int y, int x2, int y2);

			/// @brief Moves the current position to the specified point.
			/// @param x The x-coordinate of the new position.
			/// @param y The y-coordinate of the new position.
			void MoveTo(int x, int y);

			/// @brief Draws a line from the current position to the specified point.
			/// @param x The x-coordinate of the ending point.
			/// @param y The y-coordinate of the ending point.
			void LineTo(int x, int y);

			/// @brief Draws a rectangle.
			/// @param x The x-coordinate of the top-left corner.
			/// @param y The y-coordinate of the top-left corner.
			/// @param x2 The x-coordinate of the bottom-right corner.
			/// @param y2 The y-coordinate of the bottom-right corner.
			void Rect(int x, int y, int x2, int y2);

			/// @brief Draws a rectangle defined by a rectangle structure.
			/// @param r The rectangle structure defining the bounds of the rectangle.
			void Rect(const RectI32& r) { Rect(r.left, r.top, r.right, r.bottom); }

			/// @brief Draws a rectangle defined by a rectangle structure.
			/// @param r The rectangle structure defining the bounds of the rectangle.
			void Rect(const RectF& r);

			/// @brief Draws a pie-shaped wedge.
			/// @param x The x-coordinate of the bounding rectangle's top-left corner.
			/// @param y The y-coordinate of the bounding rectangle's top-left corner.
			/// @param x2 The x-coordinate of the bounding rectangle's bottom-right corner.
			/// @param y2 The y-coordinate of the bounding rectangle's bottom-right corner.
			/// @param x3 The x-coordinate of the starting point of the pie.
			/// @param y3 The y-coordinate of the starting point of the pie.
			/// @param x4 The x-coordinate of the ending point of the pie.
			/// @param y4 The y-coordinate of the ending point of the pie.
			void Pie(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4);

			/// @brief Draws a rounded rectangle.
			/// @param x The x-coordinate of the top-left corner.
			/// @param y The y-coordinate of the top-left corner.
			/// @param x2 The x-coordinate of the bottom-right corner.
			/// @param y2 The y-coordinate of the bottom-right corner.
			/// @param radius The radius of the rounded corners.
			void RoundRect(int x, int y, int x2, int y2, int radius);

			/// @brief Draws a rounded rectangle defined by a rectangle structure.
			/// @param r The rectangle structure defining the bounds of the rounded rectangle.
			/// @param radius The radius of the rounded corners.
			void RoundRect(const RectI32& r, int radius) { RoundRect(r.left, r.top, r.right, r.bottom, radius); }

			/// @brief Draws Text at the specified position.
			/// @param x The x-coordinate of the starting point.
			/// @param y The y-coordinate of the starting point.
			/// @param Text The Text to draw.
			void TextOut(int x, int y, const std::wstring& text);

			/// @brief Draws Text within a specified rectangle.
			/// @param x The x-coordinate of the top-left corner.
			/// @param y The y-coordinate of the top-left corner.
			/// @param x2 The x-coordinate of the bottom-right corner.
			/// @param y2 The y-coordinate of the bottom-right corner.
			/// @param Text The Text to draw.
			/// @param getFlags Optional getFlags for Text alignment and formatting.
			void TextRect(int x, int y, int x2, int y2, const std::wstring& text, int flags = 0);

			/// @brief Draws Text within a specified rectangle structure.
			/// @param r The rectangle structure defining the bounds for the Text.
			/// @param Text The Text to draw.
			/// @param getFlags Optional getFlags for Text alignment and formatting.
			void TextRect(const RectI32& r, const std::wstring& text, int flags = 0) { TextRect(r.left, r.top, r.right, r.bottom, text, flags); }

			/// @brief Gets the width of the specified Text.
			/// @param Text The Text to measure.
			/// @return The width of the Text in pixels.
			uint32_t TextWidth(const std::wstring& text);

			/// @brief Gets the height of the specified Text.
			/// @param Text The Text to measure.
			/// @return The height of the Text in pixels.
			uint32_t TextHeight(const std::wstring& text);

			/// @brief Measures the size of the specified Text.
			/// @param Text The Text to measure.
			/// @return The size of the Text.
			IGR_Size MeasureText(const std::wstring& text);

			/// @brief Sets the pen for drawing.
			/// @param textColor The color of the pen.
			/// @param width The width of the pen.
			/// @param style The style of the pen. Defaults to solid.
			void SetPen(const Color& textColor, int width, PenStyle style = PenStyle::Solid);

			/// @brief Sets the brush for filling shapes.
			/// @param textColor The color of the brush.
			/// @param style The style of the brush. Defaults to solid.
			void SetBrush(const Color& textColor, BrushStyle style = BrushStyle::Solid);

			/// @brief Sets the font for drawing Text.
			/// @param name The name of the font.
			/// @param size The size of the font.
			/// @param style The style of the font. Defaults to normal.
			void SetFont(const std::wstring& name, int size, FontStyle style = FontStyle::Normal);

			/// @brief Sets the opacity level.
			/// @param value The opacity value to set.
			void SetOpacity(int value);

			/// @brief Draws an image at the specified coordinates.
			/// @param x The x-coordinate.
			/// @param y The y-coordinate.
			/// @param buffer The image buffer.
			/// @param buffer_size The size of the image buffer.
			/// @param getMimeType The MIME getType of the image. Defaults to an empty string.
			void DrawImage(int x, int y, const void* buffer, size_t buffer_size, const std::wstring& mime_type = std::wstring());

			/// @brief Draws a scaled image at the specified coordinates.
			/// @param x The x-coordinate.
			/// @param y The y-coordinate.
			/// @param width The width of the image.
			/// @param height The height of the image.
			/// @param buffer The image buffer.
			/// @param buffer_size The size of the image buffer.
			/// @param getMimeType The MIME getType of the image. Defaults to an empty string.
			void DrawScaleImage(int x, int y, int width, int height, const void* buffer, size_t buffer_size, const std::wstring& mime_type = std::wstring());

			/// @brief Draws a scaled image within the specified rectangle.
			/// @param rc The rectangle specifying the area to draw the image.
			/// @param buffer The image buffer.
			/// @param buffer_size The size of the image buffer.
			/// @param getMimeType The MIME getType of the image. Defaults to an empty string.
			void DrawScaleImage(const RectI32& rc, const void* buffer, size_t buffer_size, const std::wstring& mime_type = std::wstring());

			/// @brief Rotates the image by the specified degrees.
			/// @param degrees The number of degrees to rotate the image.
			void Rotate(int degrees);

			/// @brief Resets the drawing state.
			void Reset();

			/// @brief Annotates the document with the specified JSON data.
			/// @param json The JSON data representing the annotation.
			void Annotate(const std::string& json);

			/// @brief Annotates the document with the specified annotation object.
			/// @param Annotation The annotation object.
			void Annotate(const AnnotationSerializable& Annotation);

			/// @brief Clears all bookmarks from the document.
			void ClearBookmarks();

			/// @brief Appends a bookmark to the document.
			/// @param item The bookmark to append.
			/// @param recurse Whether to append the bookmark recursively. Defaults to false.
			void AppendBookmark(const Bookmark& item, bool recurse = false);

			/// @brief Appends a bookmark to the document recursively.
			/// @param item The bookmark to append.
			void AppendBookmarkRecursive(const Bookmark& item) { AppendBookmark(item, true); }

		private:
			class impl_t;
			std::shared_ptr<impl_t> m_impl;
		};

		/// @brief Represents a bookmark item.
		class Bookmark
		{
			friend class Extractor;

		private:
			class impl_t;
			std::shared_ptr<impl_t> m_impl;
		protected:
			Bookmark(IGR_LONG doc_handle);
			Bookmark(IGR_LONG doc_handle, const IGR_Bookmark& Bookmark);
		public:
			enum class ActionType
			{
				GoTo = IGR_BOOKMARK_ACTION_GOTO,
				Link = IGR_BOOKMARK_ACTION_LINK
			};
			enum class FitType
			{
				None = IGR_BOOKMARK_FIT_NONE,
				both = IGR_BOOKMARK_FIT_BOTH,
				width = IGR_BOOKMARK_FIT_WIDTH,
				height = IGR_BOOKMARK_FIT_HEIGHT,
				rect = IGR_BOOKMARK_FIT_RECT,
			};
			enum class TextStyle
			{
				None = 0,
				Bold = IGR_TEXT_STYLE_BOLD,
				Italic = IGR_TEXT_STYLE_ITALIC,
				Underline = IGR_TEXT_STYLE_UNDERLINE,
			};
			typedef enumerable_t<Bookmark> bookmarks_t;
			typedef bookmarks_t::const_iterator const_iterator;

			Bookmark();
			Bookmark(const std::shared_ptr<impl_t>& impl);

			/// @brief Get the title of the bookmark.
			/// @return The title of the bookmark.
			std::wstring getTitle() const;

			/// @brief Get the destination of the bookmark.
			/// @return The destination of the bookmark.
			std::wstring getDestination() const;

			/// @brief Get the color of the bookmark.
			/// @return The color of the bookmark.
			Color getColor() const;

			/// @brief Get the action of the bookmark.
			/// @return The action of the bookmark.
			ActionType getAction() const;

			/// @brief Get the fit method of the bookmark.
			/// @return The fit method of the bookmark.
			FitType getFit() const;

			/// @brief Get the zoom factor of the bookmark.
			/// @return The zoom factor of the bookmark.
			double getZoom() const;

			/// @brief Get the level of the bookmark.
			/// @return The level of the bookmark.
			uint32_t getLevel() const;

			/// @brief Get the rectangle of the bookmark.
			/// @return The rectangle of the bookmark.
			RectI32 getRect() const;

			/// @brief Get the page index of the bookmark.
			/// @return The page index of the bookmark.
			uint32_t getPageIndex() const;

			/// @brief Retrieves the Text style of the bookmark.
			/// @return The current Text style of the bookmark.
			TextStyle getTextStyle() const;

			/// @brief Get the children bookmarks of the bookmark.
			/// @return The children bookmarks of the bookmark.
			const bookmarks_t& children() const;

			/// @brief Retrieves all child bookmarks.
			/// @details This function returns a constant reference to a collection of all child bookmarks.
			/// @return A constant reference to a bookmarks_t object containing all child bookmarks.
			const bookmarks_t& allChildren() const;

			/// @brief Get the first child bookmark of the bookmark.
			/// @return The first child bookmark of the bookmark.
			Bookmark getFirstChild() const;

			/// @brief Get the next sibling bookmark of the bookmark.
			/// @return The next sibling bookmark of the bookmark.
			Bookmark getNextSibling() const;

			/// @brief Conversion operator to check if the bookmark is valid.
			/// @return True if the bookmark is valid, false otherwise.
			operator bool() const { return ok(); }

			/// @brief Check if the bookmark is valid.
			/// @return True if the bookmark is valid, false otherwise.
			bool ok() const;

			/// @brief Get the handle of the bookmark.
			/// @return The handle of the bookmark.
			const IGR_Bookmark* data() const;

			/// @brief Returns an iterator to the beginning of the children collection.
			/// @details This function returns a const_iterator pointing to the first element in the children collection.
			/// @return A const_iterator to the beginning of the children collection.
			const_iterator begin() const { return children().begin(); }

			/// @brief Returns an iterator to the end of the children collection.
			/// @details This function returns a const_iterator pointing to the past-the-end element in the children collection.
			/// @return A const_iterator to the end of the children collection.
			const_iterator end() const { return children().end(); }

			/// @brief Sets the title of the bookmark.
			/// @param value The new title for the bookmark.
			/// @return Reference to the updated bookmark object.
			Bookmark& setTitle(const std::wstring& value);

			/// @brief Sets the destination of the bookmark.
			/// @param value The new destination for the bookmark.
			/// @return Reference to the updated bookmark object.
			Bookmark& setDestination(const std::wstring& value);

			/// @brief Sets the color of the bookmark.
			/// @param value The new color for the bookmark.
			/// @return Reference to the updated bookmark object.
			Bookmark& setColor(const Color value);

			/// @brief Sets the action of the bookmark.
			/// @param value The new action for the bookmark.
			/// @return Reference to the updated bookmark object.
			Bookmark& setAction(ActionType value);

			/// @brief Sets the fit getType of the bookmark.
			/// @param value The new fit getType for the bookmark.
			/// @return Reference to the updated bookmark object.
			Bookmark& setFit(FitType value);

			/// @brief Sets the zoom level of the bookmark.
			/// @param value The new zoom level for the bookmark.
			/// @return Reference to the updated bookmark object.
			Bookmark& setZoom(double value);

			/// @brief Sets the level of the bookmark in the hierarchy.
			/// @param value The new level for the bookmark.
			/// @return Reference to the updated bookmark object.
			Bookmark& setLevel(uint32_t value);

			/// @brief Sets the rectangle area of the bookmark.
			/// @param value The new rectangle area for the bookmark.
			/// @return Reference to the updated bookmark object.
			Bookmark& setRect(const RectI32& value);

			/// @brief Sets the page index of the bookmark.
			/// @param value The new page index for the bookmark.
			/// @return Reference to the updated bookmark object.
			Bookmark& setPageIndex(uint32_t value);

			/// @brief Sets the Text style of the bookmark.
			/// @param getFlags The Text style getFlags to be applied to the bookmark.
			/// @return Reference to the updated bookmark object.
			Bookmark& setTextStyle(TextStyle flags);

			/// @brief Appends a child bookmark to the current bookmark.
			/// @param child The child bookmark to be appended.
			Bookmark& AppendChild(const Bookmark& child);


			bool operator==(const Bookmark& other) const;
			bool operator!=(const Bookmark& other) const;
		};

		template<> struct EnableBitMaskOperators<Bookmark::TextStyle> { static const bool enable = true; };

		enum class PageElementType
		{
			None = 0,

			/// @brief Indicates a page element of unknown page.
			Page = IGR_PAGE_ELEMENT_TYPE_PAGE,

			/// @brief Indicates a section page element.
			Section = IGR_PAGE_ELEMENT_TYPE_SECTION,

			/// @brief Indicates a column page element.
			Column = IGR_PAGE_ELEMENT_TYPE_COLUMN,

			/// @brief Indicates a paragraph page element.
			Paragraph = IGR_PAGE_ELEMENT_TYPE_PARAGRAPH,

			/// @brief Indicates a line page element.
			Line = IGR_PAGE_ELEMENT_TYPE_LINE,

			/// @brief Indicates a run page element.
			Run = IGR_PAGE_ELEMENT_TYPE_RUN,

			/// @brief Indicates a word page element.
			Word = IGR_PAGE_ELEMENT_TYPE_WORD,

			/// @brief Indicates a character page element.
			Header = IGR_PAGE_ELEMENT_TYPE_HEADER,

			/// @brief Indicates a footer page element.
			Footer = IGR_PAGE_ELEMENT_TYPE_FOOTER,

			/// @brief Indicates an image page element.
			Image = IGR_PAGE_ELEMENT_TYPE_IMAGE,

			/// @brief Initiates a table page element.
			Table = IGR_PAGE_ELEMENT_TYPE_TABLE,

			/// @brief Initiates a table row page element.
			TableRow = IGR_PAGE_ELEMENT_TYPE_TABLE_ROW,

			/// @brief Initiates a table cell page element.
			TableCell = IGR_PAGE_ELEMENT_TYPE_TABLE_CELL,

			/// @brief Initiates a form page element.
			Form = IGR_PAGE_ELEMENT_TYPE_FORM,

			/// @brief Initiates a form element page element.
			FormElement = IGR_PAGE_ELEMENT_TYPE_FORM_ELEMENT,

			/// @brief Initiates a floating page element.
			Float = IGR_PAGE_ELEMENT_TYPE_FLOAT,

			/// @brief Initiates a graphic page element.
			Graphic = IGR_PAGE_ELEMENT_TYPE_GRAPHIC,

			/// @brief Initiates a Text box page element.
			TextBox = IGR_PAGE_ELEMENT_TYPE_TEXT_BOX,

			/// @brief Initiates a note page element.
			Notes = IGR_PAGE_ELEMENT_TYPE_NOTES,
		};


		/// @brief Represents a page element in a document.
		class PageElement
		{
			friend class Page;
		private:
			class impl_t;
			std::shared_ptr<impl_t> m_impl;
		public:
			typedef enumerable_t<PageElement> page_elements_t;
			typedef page_elements_t::const_iterator const_iterator;
			typedef std::map<std::wstring, std::wstring> style_map_t;

			/// @brief Default constructor for page_element.
			PageElement();

			/// @brief Constructs a page element with the given page handle and element.
			/// @param page_handle Handle to the page.
			/// @param element The page element.
			PageElement(IGR_HPAGE page_handle, const IGR_Page_Element& element);


			PageElement(const std::shared_ptr<impl_t>& impl);

			/// @brief Gets the getType of the page element.
			/// @return The getType of the page element.
			PageElementType getType() const;

			/// @brief Gets the bounds of the page element.
			/// @return The bounds of the page element.
			IGR_FRect getBounds() const;

			/// @brief Gets the depth of the page element.
			/// @return The depth of the page element.
			uint32_t getDepth() const;

			/// @brief Gets the getFlags of the page element.
			/// @return The getFlags of the page element.
			uint32_t getFlags() const;

			/// @brief Gets the first child of the page element.
			/// @return The first child of the page element.
			PageElement getFirstChild() const;

			/// @brief Gets the next sibling of the page element.
			/// @return The next sibling of the page element.
			PageElement getNextSibling() const;

			/// @brief Gets the style of the page element by name.
			/// @param name The name of the style.
			/// @return The style value.
			std::wstring getStyle(const std::wstring& name) const;

			/// @brief Gets the Text of the page element.
			/// @return The Text of the page element.
			std::wstring getText() const;

			/// @brief Gets all styles of the page element.
			/// @return A map of style names to style values.
			const style_map_t& getStyles() const;

			/// @brief Gets the children of the page element.
			/// @return A collection of child page elements.
			const page_elements_t& children() const;

			const page_elements_t& allChildren() const;

			/// @brief Gets an iterator to the beginning of the children collection.
			/// @return An iterator to the beginning of the children collection.
			const_iterator begin() const;

			/// @brief Gets an iterator to the end of the children collection.
			/// @return An iterator to the end of the children collection.
			const_iterator end() const;

			/// @brief Checks if the page element is valid.
			/// @return True if the page element is valid, false otherwise.
			bool ok() const;

			/// @brief Conversion operator to bool.
			/// @return True if the page element is valid, false otherwise.
			operator bool() const { return ok(); }

			/// @brief Gets the handle of the page element.
			/// @return The handle of the page element.
			const IGR_Page_Element* data() const;

			bool operator==(const PageElement& other) const;
		};

		/// @brief Represents the pixel data of a page.
		class PagePixels
		{
			friend class Page;
		protected:
			/// @brief Constructs a PagePixels object.
			/// @param page_handle The handle to the page.
			/// @param pixels The pixel data of the page.
			PagePixels(IGR_HPAGE page_handle, const IGR_Page_Pixels& pixels);
		public:
			/// @brief Gets the pixel data.
			/// @return A pointer to the pixel data.
			const IGR_Page_Pixels* data() const;

			/// @brief Gets the width of the page in pixels.
			/// @return The width of the page.
			size_t getWidth() const;

			/// @brief Gets the height of the page in pixels.
			/// @return The height of the page.
			size_t getHeight() const;

			/// @brief Gets the stride of the pixel data.
			/// @return The stride of the pixel data.
			size_t getStride() const;

			/// @brief Gets the pixel getType.
			/// @return The pixel getType.
			PixelType getType() const;

			/// @brief Gets the getFlags associated with the pixel data.
			/// @return The getFlags.
			uint32_t getFlags() const;

			/// @brief Gets a pointer to the raw pixel data.
			/// @return A pointer to the raw pixel data.
			const void* getData() const;

			/// @brief Gets a pointer to the pixel data of a specific row.
			/// @param row The row index.
			/// @return A pointer to the pixel data of the specified row.
			const void* getRow(size_t row) const;

			/// @brief Gets the number of colors in the pixel data.
			/// @return The number of colors.
			size_t getColorCount() const;

			/// @brief Gets the color at the specified index.
			/// @param index The color index.
			/// @return The color at the specified index.
			Color getColor(size_t index) const;

		private:
			/// @brief Implementation details of the PagePixels class.
			class impl_t;
			/// @brief Shared pointer to the implementation details.
			std::shared_ptr<impl_t> m_impl;
		};


		enum class FormElementType
		{
			Button = IGR_PAGE_FORM_ELEMENT_TYPE_BUTTON,
			RadioButton = IGR_PAGE_FORM_ELEMENT_TYPE_RADIOBUTTON,
			Checkbox = IGR_PAGE_FORM_ELEMENT_TYPE_CHECKBOX,
			FileSelect = IGR_PAGE_FORM_ELEMENT_TYPE_FILESELECT,
			MultilineText = IGR_PAGE_FORM_ELEMENT_TYPE_MULTILINE_TEXT,
			SinglelineText = IGR_PAGE_FORM_ELEMENT_TYPE_SINGLELINE_TEXT,
			Combobox = IGR_PAGE_FORM_ELEMENT_TYPE_COMBOBOX,
			Listbox = IGR_PAGE_FORM_ELEMENT_TYPE_LISTBOX,
			Signature = IGR_PAGE_FORM_ELEMENT_TYPE_SIGNATURE,
			Password = IGR_PAGE_FORM_ELEMENT_TYPE_PASSWORD,
		};

		enum class FormElementFlag : uint32_t
		{
			ReadOnly = IGR_PAGE_FORM_ELEMENT_FLAG_READONLY,
			Required = IGR_PAGE_FORM_ELEMENT_FLAG_REQUIRED,
			Comb = IGR_PAGE_FORM_ELEMENT_FLAG_COMB,
			Checked = IGR_PAGE_FORM_ELEMENT_FLAG_CHECKED
		};
		template<> struct EnableBitMaskOperators<FormElementFlag> { static const bool enable = true; };

		/// @brief Represents a form element on a page.
		class FormElement
		{
		protected:
		public:
			/// @brief Represents an option within a form element.
			class Option
			{
			public:
				/// @brief Constructs an Option object.
				/// @param name The name of the option.
				/// @param value The value of the option.
				/// @param selected Whether the option is selected.
				Option(const std::wstring& name, const std::wstring& value, bool selected);

				/// @brief Gets the name of the option.
				/// @return The name of the option.
				const std::wstring& getName() const { return m_name; }

				/// @brief Gets the value of the option.
				/// @return The value of the option.
				const std::wstring& getValue() const { return m_value; }

				/// @brief Checks if the option is selected.
				/// @return True if the option is selected, false otherwise.
				bool getSelected() const { return m_selected; }

			private:
				std::wstring m_name; ///< The name of the option.
				std::wstring m_value; ///< The value of the option.
				bool m_selected; ///< Whether the option is selected.
			};

			typedef std::vector<Option> options_t;
			using Type = FormElementType;
			using Flags = FormElementFlag;

			/// @brief Constructs a FormElement object from a page form element.
			/// @param element The page form element.
			FormElement(const IGR_Page_Form_Element& element);

			/// @brief Default constructor for FormElement.
			FormElement();

			/// @brief Gets the getType of the form element.
			/// @return The getType of the form element.
			FormElementType get_type() const;

			/// @brief Gets the getFlags of the form element.
			/// @return The getFlags of the form element.
			uint32_t getFlags() const;

			/// @brief Checks if a specific flag is set for the form element.
			/// @param flag The flag to check.
			/// @return True if the flag is set, false otherwise.
			bool getFlag(FormElementFlag flag) const { return (getFlags() & static_cast<uint32_t>(flag)) != 0; }

			/// @brief Gets the X coordinate of the form element.
			/// @return The X coordinate.
			int32_t getX() const;

			/// @brief Gets the Y coordinate of the form element.
			/// @return The Y coordinate.
			int32_t getY() const;

			/// @brief Gets the width of the form element.
			/// @return The width.
			int32_t getWidth() const;

			/// @brief Gets the height of the form element.
			/// @return The height.
			int32_t getHeight() const;

			/// @brief Gets the bounds of the form element.
			/// @return The bounds as an RectI32.
			RectI32 getBounds() const;

			/// @brief Gets the name of the form element.
			/// @return The name.
			std::wstring getName() const;

			/// @brief Gets the value of the form element.
			/// @return The value.
			std::wstring getValue() const;

			/// @brief Gets the caption of the form element.
			/// @return The caption.
			std::wstring getCaption() const;

			/// @brief Gets the font name of the form element.
			/// @return The font name.
			std::wstring getFontName() const;

			/// @brief Gets the action associated with the form element.
			/// @return The action.
			std::wstring getAction() const;

			/// @brief Gets the action destination associated with the form element.
			/// @return The action destination.
			std::wstring getActionDest() const;

			/// @brief Gets the font size of the form element.
			/// @return The font size.
			int getFontSize() const;

			/// @brief Gets the selected index of the form element.
			/// @return The selected index.
			int getSelected() const;

			/// @brief Checks if the form element is checked.
			/// @return True if the form element is checked, false otherwise.
			bool getChecked() const { return getFlag(FormElementFlag::Checked); }

			/// @brief Gets the options of the form element.
			/// @return A reference to the options.
			const options_t& getOptions() const;

		private:
			/// @brief Gets a string representation of a specific getType.
			/// @param getType The getType to get the string representation for.
			/// @return The string representation.
			std::wstring get_str(int type) const;

			/// @brief Implementation details of the FormElement class.
			class impl_t;

			/// @brief Shared pointer to the implementation details.
			std::shared_ptr<impl_t> m_impl;
		};

		/// @brief Represents a hyperlink element in a document.
		class Hyperlink
		{
		public:
			/// @brief Enum class for hyperlink types.
			enum class Type
			{
				Uri = IGR_HYPERLINK_ACTION_URI, ///< URI action getType.
				GoTo = IGR_HYPERLINK_ACTION_GOTO, ///< Go-to action getType.
			};

			/// @brief Enum class for hyperlink getFlags.
			enum class Flags : uint32_t
			{
				ChangesLeft = IGR_HYPERLINK_FLAGS_CHANGE_LEFT, ///< Flag indicating change in left position.
				ChangesTop = IGR_HYPERLINK_FLAGS_CHANGE_TOP, ///< Flag indicating change in top position.
				ChangesZoom = IGR_HYPERLINK_FLAGS_CHANGE_ZOOM, ///< Flag indicating change in zoom level.
			};

			/// @brief Enum class for hyperlink fit types.
			enum class Fit
			{
				Xyz = IGR_HYPERLINK_FIT_XYZ, ///< XYZ fit getType.
				Fit = IGR_HYPERLINK_FIT_FIT, ///< Fit getType.
				Fith = IGR_HYPERLINK_FIT_FITH, ///< Fit height getType.
				Fitv = IGR_HYPERLINK_FIT_FITV, ///< Fit width getType.
				Fitr = IGR_HYPERLINK_FIT_FITR, ///< Fit rectangle getType.
				Fitb = IGR_HYPERLINK_FIT_FITB, ///< Fit bounding box getType.
				Fitbh = IGR_HYPERLINK_FIT_FITBH, ///< Fit bounding box height getType.
				Fitbv = IGR_HYPERLINK_FIT_FITBV, ///< Fit bounding box width getType.
			};

			/// @brief Constructs a Hyperlink object from an IGR_Hyperlink structure.
			/// @param Hyperlink The IGR_Hyperlink structure.
			Hyperlink(const IGR_Hyperlink& Hyperlink);

			/// @brief Default constructor for Hyperlink.
			Hyperlink();

			/// @brief Gets the getType of the hyperlink.
			/// @return The getType of the hyperlink.
			Type get_type() const;

			/// @brief Gets the X coordinate of the hyperlink.
			/// @return The X coordinate.
			int getX() const;

			/// @brief Gets the Y coordinate of the hyperlink.
			/// @return The Y coordinate.
			int getY() const;

			/// @brief Gets the width of the hyperlink.
			/// @return The width.
			int getWidth() const;

			/// @brief Gets the height of the hyperlink.
			/// @return The height.
			int getHeight() const;

			/// @brief Gets the bounds of the hyperlink as a rectangle.
			/// @return The bounds as an RectI32.
			RectI32 getBounds() const;

			/// @brief Gets the destination fit getType of the hyperlink.
			/// @return The destination fit getType.
			Fit getDestFit() const;

			/// @brief Gets the page number the hyperlink points to.
			/// @return The page number.
			int getPageNumber() const;

			/// @brief Gets the left coordinate of the destination.
			/// @return The left coordinate.
			int getDestLeft() const;

			/// @brief Gets the top coordinate of the destination.
			/// @return The top coordinate.
			int getDestTop() const;

			/// @brief Gets the right coordinate of the destination.
			/// @return The right coordinate.
			int getDestRight() const;

			/// @brief Gets the bottom coordinate of the destination.
			/// @return The bottom coordinate.
			int getDestBottom() const;

			/// @brief Gets the destination rectangle of the hyperlink.
			/// @return The destination rectangle as an RectI32.
			RectI32 getDestRect() const;

			/// @brief Gets the getFlags of the hyperlink.
			/// @return The getFlags.
			Flags getFlags() const;

			/// @brief Checks if a specific flag is set.
			/// @param flag The flag to check.
			/// @return True if the flag is set, false otherwise.
			bool getFlag(Flags flag) const;

			/// @brief Gets the reference string of the hyperlink.
			/// @return The reference string.
			std::wstring getRef() const;

			/// @brief Gets the URI of the hyperlink.
			/// @return The URI.
			std::wstring getUri() const;

		private:
			/// @brief Implementation details for the Hyperlink class.
			class impl_t;

			/// @brief Shared pointer to the implementation.
			std::shared_ptr<impl_t> m_impl;
		};

		template<> struct EnableBitMaskOperators<Hyperlink::Flags> { static const bool enable = true; };

		/// @brief Represents style information for OCR (Optical Character Recognition) text, including font family, font size, and text style.
		class OcrStyleInfo
		{
		public:
			enum class TextStyle
			{
				None = 0,
				Bold = IGR_TEXT_STYLE_BOLD,
				Italic = IGR_TEXT_STYLE_ITALIC,
				Underline = IGR_TEXT_STYLE_UNDERLINE,
			};

			/// @brief Default constructor for OcrStyleInfo.
			OcrStyleInfo();

			/// @brief Retrieves the font family as a wide string.
			/// @return A wide string representing the font family.
			std::wstring getFontFamily() const;

			/// @brief Sets the font family for text rendering.
			/// @param fontFamily A wide string representing the name of the font family to set.
			OcrStyleInfo& setFontFamily(const std::wstring& fontFamily);

			/// @brief Retrieves the current font size.
			/// @return The font size as a floating-point number.
			float getFontSize() const;

			/// @brief Sets the font size for text rendering.
			/// @param fontSize The desired font size, specified as a floating-point value.
			OcrStyleInfo& setFontSize(float fontSize);

			/// @brief Retrieves the text style associated with the object.
			/// @return The text style as a TextStyle object.
			TextStyle getTextStyle() const;

			/// @brief Sets the text style for rendering text.
			/// @param textStyle The text style to apply, such as font, size, or weight.
			OcrStyleInfo& setTextStyle(TextStyle textStyle);

			/// @brief Retrieves a constant reference to the raw OCR image style information.
			/// @return A constant reference to an object of type IGR_Open_Callback_Action_OCR_Image_Style_Info representing the raw OCR image style information.
			const IGR_Open_Callback_Action_OCR_Image_Style_Info& raw() const;
		private:
			IGR_Open_Callback_Action_OCR_Image_Style_Info m_style{};
		};

		template<> struct EnableBitMaskOperators<OcrStyleInfo::TextStyle> { static const bool enable = true; };

		/// @brief Represents an OCR image object with methods for managing image data, blocks, text, and orientation.
		class OcrImage
		{
		public:
			/// @brief Processes an OCR image using the provided callback action.
			/// @param dest A pointer to an IGR_Open_Callback_Action_OCR_Image object that will handle the OCR image processing.
			OcrImage(IGR_Open_Callback_Action_OCR_Image* dest);

			/// @brief Retrieves a raw pointer to an OCR image callback action.
			/// @return A constant pointer to an `IGR_Open_Callback_Action_OCR_Image` object.
			const IGR_Open_Callback_Action_OCR_Image* raw() const;

			/// @brief Retrieves a pointer to the pixel data information of the image.
			/// @return A constant pointer to an IGR_Open_DIB_Info structure containing the pixel data information.
			const IGR_Open_DIB_Info* getPixelData() const;

			/// @brief Retrieves the index of the source page.
			/// @return The index of the source page as a size_t value.
			size_t getSourcePageIndex() const;

			/// @brief Retrieves the source rectangle.
			/// @return A Rect object representing the source rectangle.
			Rect getSourceRect() const;

			/// @brief Saves an image to a file with the specified filename and MIME type.
			/// @param filename The name of the file where the image will be saved, including its path.
			/// @param mimeType The MIME type of the image format to use for saving (e.g., 'image/png', 'image/jpeg').
			void SaveImage(const std::wstring& filename, const std::wstring& mimeType) const;

			/// @brief Begins a block of a specified type and associates it with a rectangular region.
			/// @param blockType An integer representing the type of block to start.
			/// @param rect A constant reference to a Rect object defining the rectangular region associated with the block.
			void StartBlock(int blockType, const Rect& rect) const;

			/// @brief Starts a block of a specified type and associates it with a quadrilateral point.
			/// @param blockType The type of block to start, represented as an integer.
			/// @param quad A constant reference to an IGR_QuadPoint object representing the quadrilateral point associated with the block.
			void StartBlock(int blockType, const IGR_QuadPoint& quad) const;

			/// @brief Ends a block of the specified type.
			/// @param blockType The type of block to end, represented as an integer.
			void EndBlock(int blockType) const;

			/// @brief Adds text to a specified rectangular area with optional styling and flags.
			/// @param text The text to be added, represented as a wide string.
			/// @param rect The rectangular area where the text will be added.
			/// @param flags Optional flags that modify the behavior of the text addition. Defaults to 0.
			/// @param style Optional style information for the text, represented as an OcrStyleInfo object. Defaults to a default-constructed OcrStyleInfo object.
			void AddText(const std::wstring& text, const Rect& rect, uint32_t flags = 0, const OcrStyleInfo& style = OcrStyleInfo()) const;

			/// @brief Adds a text element to a specified quadrilateral region with optional styling and flags.
			/// @param text The text to be added, represented as a wide string.
			/// @param quad The quadrilateral region where the text will be placed.
			/// @param flags Optional flags to modify the behavior of the text addition. Defaults to 0.
			/// @param style Optional styling information for the text. Defaults to an instance of OcrStyleInfo with default values.
			void AddText(const std::wstring& text, const IGR_QuadPoint& quad, uint32_t flags = 0, const OcrStyleInfo& style = OcrStyleInfo()) const;

			/// @brief Adjusts the orientation of an object by a specified angle.
			/// @param angle The angle, in degrees, by which to reorient the object. 
			void Reorient(float angle) const;
		private:
			IGR_Open_Callback_Action_OCR_Image* m_dest;
		};

		// --------------------------------------------------------------------------------

#define DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(TYPE) \
		std::string serialize() const override; \
		void bind(const IGR_Annotation& anno) override; \
		bool operator==(const TYPE& other) const; \
		bool operator!=(const TYPE& other) const { return !(*this == other); } 
#define DOCFILTERS_ANNOTATION_CONSTRUCT(CLASSTYPE, ANNOTYPE)\
	CLASSTYPE(): Annotation(ANNOTYPE) {} \
	DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(CLASSTYPE)
#define DOCFILTERS_OBJECT_SERIALIZE_METHODS(TYPE) \
		std::string serialize() const override; \
		bool operator==(const TYPE& other) const; \
		bool operator!=(const TYPE& other) const { return !(*this == other); } 
#define DOCFILTERS_ANNOTATION_PROPERTY(TYPE, NAME, DEFAULT_VALUE) \
	private: TYPE m_##NAME = DEFAULT_VALUE; \
	public: const TYPE& get##NAME() const { return m_##NAME; } \
	public: TYPE& get##NAME() { return m_##NAME; } \
	public: TYPE& set##NAME(const TYPE& value) { m_##NAME = value; return m_##NAME; } 

		class AnnotationBase : public AnnotationSerializable
		{
			friend AnnotationBase& json_bind(const AnnoBind& src, const std::wstring& name, AnnotationBase& dest);
			friend JsonWriter& json_write_props(JsonWriter& writer, const AnnotationBase& src);
		public:
			enum class Flags : uint32_t
			{
				None = 0,
				Invisible = 0x1,
				Hidden = 0x2,
				Print = 0x4,
				NoZoom = 0x8,
				NoRotate = 0x10,
				NoView = 0x20,
				ReadOnly = 0x40,
				Locked = 0x80,
				ToogleNoView = 0x100,
				LockedContents = 0x200
			};

			enum class Type
			{
				Unknown = IGR_ANNOTATION_UNKNOWN,
				Text = IGR_ANNOTATION_TEXT,
				Link = IGR_ANNOTATION_LINK,
				Freetext = IGR_ANNOTATION_FREETEXT,
				Line = IGR_ANNOTATION_LINE,
				Rectangle = IGR_ANNOTATION_SQUARE,
				Ellipse = IGR_ANNOTATION_CIRCLE,
				Polygon = IGR_ANNOTATION_POLYGON,
				Polyline = IGR_ANNOTATION_POLYLINE,
				Highlight = IGR_ANNOTATION_HIGHLIGHT,
				Underline = IGR_ANNOTATION_UNDERLINE,
				Squiggly = IGR_ANNOTATION_SQUIGGLE,
				Strikeout = IGR_ANNOTATION_STRIKEOUT,
				Stamp = IGR_ANNOTATION_STAMP,
				Caret = IGR_ANNOTATION_CARET,
				Ink = IGR_ANNOTATION_INK,
				Popup = IGR_ANNOTATION_POPUP,
				FileAttachment = IGR_ANNOTATION_FILEATTACHMENT,
				Sound = IGR_ANNOTATION_SOUND,
				Movie = IGR_ANNOTATION_MOVIE,
				Widget = IGR_ANNOTATION_WIDGET,
				Screen = IGR_ANNOTATION_SCREEN,
				Printermark = IGR_ANNOTATION_PRINTERMARK,
				Trapnet = IGR_ANNOTATION_TRAPNET,
				Watermark = IGR_ANNOTATION_WATERMARK,
				ThreeD = IGR_ANNOTATION_3D,
				Barcode = IGR_ANNOTATION_BARCODE,
				NamedDestination = IGR_ANNOTATION_NAMED_DESTINATION,
			};

			enum class BorderStyleType
			{
				Unknown = 0,
				Solid = 1,
				Dashed = 2,
				Beveled = 3,
				Inset = 4,
				Underline = 5,
				RightOpenArrow = 7,
				RightClosedArrow = 8,
				Slash = 9
			};

			enum class LineEndingType
			{
				None = 0,
				Square = 1,
				Circle = 2,
				Diamond = 3,
				OpenArrow = 4,
				ClosedArrow = 5,
				Butt = 6,
				RightOpenArrow = 7,
				RightClosedArrow = 8,
				Slash = 9
			};

			enum class AlignmentType
			{
				Left = 0,
				Center = 1,
				Right = 2
			};

			class LineEndings : public AnnotationSerializable
			{
			public:
				LineEndings() = default;

				DOCFILTERS_ANNOTATION_PROPERTY(LineEndingType, Begin, LineEndingType::None);
				DOCFILTERS_ANNOTATION_PROPERTY(LineEndingType, End, LineEndingType::None);

				DOCFILTERS_OBJECT_SERIALIZE_METHODS(LineEndings)
			};

			class BorderStyle : public AnnotationSerializable
			{
			public:
				BorderStyle() = default;

				DOCFILTERS_ANNOTATION_PROPERTY(BorderStyleType, Type, BorderStyleType::Solid);
				DOCFILTERS_ANNOTATION_PROPERTY(int, Width, 1);
				DOCFILTERS_ANNOTATION_PROPERTY(int, Intensity, 0);
				DOCFILTERS_ANNOTATION_PROPERTY(std::vector<int>, Dash, std::vector<int>());

				DOCFILTERS_OBJECT_SERIALIZE_METHODS(BorderStyle);
			};

			class DefaultAppearance : public AnnotationSerializable
			{
			public:
				DefaultAppearance() = default;

				DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, FontName, L"Helvetica");
				DOCFILTERS_ANNOTATION_PROPERTY(double, FontSize, 14);
				DOCFILTERS_ANNOTATION_PROPERTY(Color, TextColor, Color::black());

				DOCFILTERS_OBJECT_SERIALIZE_METHODS(DefaultAppearance)
			};

			class AppearanceStream : public AnnotationSerializable
			{
			public:
				DOCFILTERS_ANNOTATION_PROPERTY(std::string, Content, std::string());
				DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, ContentType, std::wstring());
				DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Encoding, L"Base64");

				void load_from_bytes(const void* buffer, size_t size)
				{
					setContent(base64_encode(buffer, size));
				}

				bool empty() const
				{
					return getContent().empty();
				}

				DOCFILTERS_OBJECT_SERIALIZE_METHODS(AppearanceStream)
			};

			class AppearanceStreams : public AnnotationSerializable
			{
			public:
				DOCFILTERS_ANNOTATION_PROPERTY(AppearanceStream, Normal, AppearanceStream());
				DOCFILTERS_ANNOTATION_PROPERTY(AppearanceStream, Rollover, AppearanceStream());
				DOCFILTERS_ANNOTATION_PROPERTY(AppearanceStream, Down, AppearanceStream());

				bool empty() const
				{
					return getNormal().empty() && getRollover().empty() && getDown().empty();
				}

				DOCFILTERS_OBJECT_SERIALIZE_METHODS(AppearanceStreams);
			};

			AnnotationBase(Type type = Type::Unknown)
				: type(type) {
			}
			virtual ~AnnotationBase() = default;

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(Flags, Flags, Flags::Print);
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Name, std::wstring());
			DOCFILTERS_ANNOTATION_PROPERTY(DateTime, DateModified, DateTime());
			DOCFILTERS_ANNOTATION_PROPERTY(Color, Color, Color::transparent());
			DOCFILTERS_ANNOTATION_PROPERTY(RectI32, Rect, RectI32());
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Text, std::wstring());
			DOCFILTERS_ANNOTATION_PROPERTY(std::shared_ptr<BorderStyle>, Border, nullptr);

			const Type& getType() const { return type; }
			Type& getType() { return type; }

			DOCFILTERS_OBJECT_SERIALIZE_METHODS(AnnotationBase);
		protected:
			Type type = Type::Unknown;
		};
		template<> struct EnableBitMaskOperators<AnnotationBase::Flags> { static const bool enable = true; };

		template <typename AnnotationBase::Type Type>
		class AnnotationTypeT
		{
		public:
			static const AnnotationBase::Type AnnoType = Type;
		};

		using AnnotationBorderStyle = AnnotationBase::BorderStyle;

		class AnnotationNote
			: public AnnotationBase
			, public AnnotationTypeT<AnnotationBase::Type::Text>
		{
		public:
			AnnotationNote() : AnnotationBase() {}
			DOCFILTERS_OBJECT_SERIALIZE_METHODS(AnnotationNote);

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, State, std::wstring());
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, StateModel, std::wstring());
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Author, std::wstring());

			static AnnotationNote CreateReply(const std::wstring& text
				, const std::wstring& author = std::wstring()
				, Flags flags = Flags::Print)
			{
				AnnotationNote note;
				note.setText(text);
				note.setAuthor(author);
				note.setFlags(flags);
				return note;
			}
		};

		class Annotation
			: public AnnotationBase
		{
		public:
			Annotation(Type type = Type::Unknown) : AnnotationBase(type) {}
			static std::unique_ptr<Annotation> make(const IGR_Annotation& anno);

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(Annotation);

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Subject, std::wstring());
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Title, std::wstring());
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Intent, std::wstring());
			DOCFILTERS_ANNOTATION_PROPERTY(double, Opacity, 1);
			DOCFILTERS_ANNOTATION_PROPERTY(DateTime, DateCreated, DateTime());
			DOCFILTERS_ANNOTATION_PROPERTY(std::shared_ptr<DefaultAppearance>, DefaultAppearance, nullptr);
			DOCFILTERS_ANNOTATION_PROPERTY(AppearanceStreams, Appearance, AppearanceStreams());
			DOCFILTERS_ANNOTATION_PROPERTY(std::vector<AnnotationNote>, Replies, std::vector<AnnotationNote>());
			DOCFILTERS_ANNOTATION_PROPERTY(std::vector<Point>, Points, std::vector<Point>());
			DOCFILTERS_ANNOTATION_PROPERTY(std::shared_ptr<AnnotationPopup>, Popup, nullptr);

			static bool is_equal(const Annotation& a, const Annotation& b);
			bool is_equal(const Annotation& other) const;
		};

		class AnnotationPopup
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::Popup>
		{
		public:
			DOCFILTERS_ANNOTATION_CONSTRUCT(AnnotationPopup, AnnotationBase::Type::Popup);

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(bool, Open, false);
		};

		class AnnotationAction
			: public AnnotationBase
		{
		public:
			enum class ActionType
			{
				Unknown,
				GoTo,
				Uri,
				GoToRemote,
				Named
			};
			DOCFILTERS_OBJECT_SERIALIZE_METHODS(AnnotationAction);

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(ActionType, Type, ActionType::Unknown);
			DOCFILTERS_ANNOTATION_PROPERTY(bool, NewWindow, false);
			DOCFILTERS_ANNOTATION_PROPERTY(int, Page, 0);
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Uri, std::wstring());
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Name, std::wstring());
			DOCFILTERS_ANNOTATION_PROPERTY(double, Zoom, 0);
			DOCFILTERS_ANNOTATION_PROPERTY(RectI32, Rect, RectI32());
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Filename, std::wstring());
		};

		class AnnotationStickyNote
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::Text>
		{
		public:
			DOCFILTERS_ANNOTATION_CONSTRUCT(AnnotationStickyNote, AnnotationBase::Type::Text);

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, State, std::wstring())
				DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, StateModel, std::wstring())
				DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Author, std::wstring())
				DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, IconName, std::wstring())
				DOCFILTERS_ANNOTATION_PROPERTY(bool, Open, false)
		};

		class AnnotationLink
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::Link>

		{
		public:
			enum class HighlightType
			{
				None = 0,
				Invert = 1,
				Outline = 2,
				Push = 3,
			};

			DOCFILTERS_ANNOTATION_CONSTRUCT(AnnotationLink, AnnotationBase::Type::Link);

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(HighlightType, Highlight, HighlightType::None);
			DOCFILTERS_ANNOTATION_PROPERTY(AnnotationAction, Action, AnnotationAction());

			void setUri(const std::wstring& uri)
			{
				getAction().setType(AnnotationAction::ActionType::Uri);
				getAction().setUri(uri);
			}
			void setGoTo(int page)
			{
				getAction().setType(AnnotationAction::ActionType::GoTo);
				getAction().setPage(page);
			}
		};

		class AnnotationFreeText
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::Freetext>
		{
		public:
			DOCFILTERS_ANNOTATION_CONSTRUCT(AnnotationFreeText, AnnotationBase::Type::Freetext);

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(AlignmentType, Alignment, AlignmentType::Left);
		};

		class AnnotationLine
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::Line>
		{
		public:
			DOCFILTERS_ANNOTATION_CONSTRUCT(AnnotationLine, AnnotationBase::Type::Line);

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(LineEndings, LineEndings, LineEndings());
			DOCFILTERS_ANNOTATION_PROPERTY(Color, InteriorColor, Color::transparent());
		};

		class AnnotationRectangle
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::Rectangle>
		{
		public:
			DOCFILTERS_ANNOTATION_CONSTRUCT(AnnotationRectangle, AnnotationBase::Type::Rectangle);

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(LineEndings, LineEndings, LineEndings());
			DOCFILTERS_ANNOTATION_PROPERTY(Color, InteriorColor, Color::transparent());
			DOCFILTERS_ANNOTATION_PROPERTY(RectI32, RectDifferences, RectI32());
		};

		class AnnotationEllipse
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::Ellipse>
		{
		public:
			DOCFILTERS_ANNOTATION_CONSTRUCT(AnnotationEllipse, AnnotationBase::Type::Ellipse);

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(Color, InteriorColor, Color::transparent());
			DOCFILTERS_ANNOTATION_PROPERTY(RectI32, RectDifferences, RectI32());
		};

		class AnnotationPolygon
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::Polygon>
		{
		public:
			DOCFILTERS_ANNOTATION_CONSTRUCT(AnnotationPolygon, AnnotationBase::Type::Polygon);

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(Color, InteriorColor, Color::transparent());
		};

		class AnnotationPolyline
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::Polyline>
		{
		public:
			DOCFILTERS_ANNOTATION_CONSTRUCT(AnnotationPolyline, AnnotationBase::Type::Polyline);

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(LineEndings, LineEndings, LineEndings());
		};

		class AnnotationMarkup : public Annotation
		{
		public:
			AnnotationMarkup(Type type = Type::Unknown) : Annotation(type) {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationMarkup)
		};


		class AnnotationHighlight
			: public AnnotationMarkup
			, public AnnotationTypeT<AnnotationBase::Type::Highlight>
		{
		public:
			AnnotationHighlight() : AnnotationMarkup(Type::Highlight) {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationHighlight)
		};

		class AnnotationSquiggly
			: public AnnotationMarkup
			, public AnnotationTypeT<AnnotationBase::Type::Squiggly>
		{
		public:
			AnnotationSquiggly() : AnnotationMarkup(Type::Squiggly) {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationSquiggly)
		};

		class AnnotationStrikeOut
			: public AnnotationMarkup
			, public AnnotationTypeT<AnnotationBase::Type::Strikeout>
		{
		public:
			AnnotationStrikeOut() : AnnotationMarkup(Type::Strikeout) {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationStrikeOut)
		};

		class AnnotationUnderline
			: public AnnotationMarkup
			, public AnnotationTypeT<AnnotationBase::Type::Underline>
		{
		public:
			AnnotationUnderline() : AnnotationMarkup(Type::Underline) {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationUnderline)
		};

		class AnnotationStamp
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::Stamp>
		{
		public:
			AnnotationStamp() : Annotation(Type::Stamp) {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationStamp)
		};

		class AnnotationInk
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::Ink>
		{
		public:
			DOCFILTERS_ANNOTATION_CONSTRUCT(AnnotationInk, AnnotationBase::Type::Ink)

				// Properties
				DOCFILTERS_ANNOTATION_PROPERTY(std::vector<std::vector<Point>>, InkList, std::vector<std::vector<Point>>());
		};

		class AnnotationBarcode
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::Barcode>
		{
		public:
			DOCFILTERS_ANNOTATION_CONSTRUCT(AnnotationBarcode, AnnotationBase::Type::Barcode);

			AnnotationBarcode(const std::wstring& subType)
				: Annotation(Type::Barcode)
				, m_SubType(subType)
			{
			}

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, SubType, std::wstring());
			DOCFILTERS_ANNOTATION_PROPERTY(Color, BgColor, Color::transparent());
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Caption, std::wstring());
			DOCFILTERS_ANNOTATION_PROPERTY(std::wstring, Content, std::wstring());
			DOCFILTERS_ANNOTATION_PROPERTY(int, ErrorCorrectionLevel, 3);
			DOCFILTERS_ANNOTATION_PROPERTY(int, Margin, 0);
		};

		class AnnotationQrCode
			: public AnnotationBarcode
		{
		public:
			AnnotationQrCode() : AnnotationBarcode(L"qr") {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationQrCode)
		};

		class AnnotationAztec
			: public AnnotationBarcode
		{
		public:
			AnnotationAztec() : AnnotationBarcode(L"aztec") {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationAztec)
		};

		class AnnotationDatamatrix
			: public AnnotationBarcode
		{
		public:
			AnnotationDatamatrix() : AnnotationBarcode(L"datamatrix") {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationDatamatrix)
		};

		class AnnotationPDF417
			: public AnnotationBarcode
		{
		public:
			AnnotationPDF417() : AnnotationBarcode(L"pdf417") {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationPDF417)
		};

		class AnnotationCode39
			: public AnnotationBarcode
		{
		public:
			AnnotationCode39() : AnnotationBarcode(L"code39") {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationCode39)
		};

		class AnnotationCode93
			: public AnnotationBarcode
		{
		public:
			AnnotationCode93() : AnnotationBarcode(L"code93") {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationCode93)
		};

		class AnnotationCode128
			: public AnnotationBarcode
		{
		public:
			AnnotationCode128() : AnnotationBarcode(L"code128") {}

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationCode128)
		};

		class AnnotationGS1_128
			: public AnnotationBarcode
		{
		public:
			AnnotationGS1_128() : AnnotationBarcode(L"gs1-128") {}

			using PartMap = std::map<std::wstring, std::wstring>;

			DOCFILTERS_ANNOTATION_SERIALIZE_METHODS(AnnotationGS1_128);

			void AddPart(const std::wstring& name, const std::wstring& value) { m_Parts[name] = value; }

			// Properties
			DOCFILTERS_ANNOTATION_PROPERTY(bool, AutoCaption, false);
			DOCFILTERS_ANNOTATION_PROPERTY(PartMap, Parts, PartMap());
		};

		class AnnotationNamedDestination
			: public Annotation
			, public AnnotationTypeT<AnnotationBase::Type::NamedDestination>
		{
		public:
			DOCFILTERS_ANNOTATION_CONSTRUCT(AnnotationNamedDestination, Type::NamedDestination);
		};


		inline std::ostream& operator<<(std::ostream& os, const DateTime& val)
		{
			return os << val.ToIsoString();
		}
		inline std::ostream& operator<<(std::ostream& os, const Color& val)
		{
			return os << "{ "
				<< static_cast<uint32_t>(val.r) << ","
				<< static_cast<uint32_t>(val.g) << ","
				<< static_cast<uint32_t>(val.b) << "}";
		}
		inline std::ostream& operator<<(std::ostream& os, const Point& val)
		{
			return os << "{ " << val.x() << "," << val.y() << "}";
		}
		inline std::ostream& operator<<(std::ostream& os, const RectI32& val)
		{
			return os << "{ left: " << val.left << ", top: " << val.top << ", right: " << val.right << ", bottom: " << val.bottom << " }";
		}
		inline std::ostream& operator<<(std::ostream& os, const AnnotationSerializable& val)
		{
			return os << val.serialize();
		}
		template <typename T> inline std::ostream& operator<<(std::ostream& os, const std::optional<T>& val) {
			if (val.has_value())
				os << *val;
			else
				os << "{{empty}}";
			return os;
		}

	} // namespace DocFilters
} // namespace Hyland

#endif // !DOCFILTERS_OBJECTS_H
