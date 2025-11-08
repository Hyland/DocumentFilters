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

#include <stack>
#include <string>
#include <utility>
#include <vector>

namespace Hyland
{
	namespace DocFilters
	{
		/**
		 * @brief Copies a std::string to a destination character array.
		 *
		 * @tparam size The size of the destination array.
		 * @param str The source string to copy.
		 * @param dest The destination character array.
		 * @throws std::runtime_error if the source string is too long to fit in the destination array.
		 */
		template <size_t size>
		inline void copy_string(const std::string& str, char(&dest)[size])
		{
			if (str.size() >= size)
				throw std::runtime_error("String too long");

			std::copy(str.begin(), str.end(), dest);
			dest[str.size()] = 0;
		}

		/**
		 * @brief Copies a std::u16string to a destination IGR_UCS2 array.
		 *
		 * @tparam size The size of the destination array.
		 * @param str The source string to copy.
		 * @param dest The destination IGR_UCS2 array.
		 * @throws std::runtime_error if the source string is too long to fit in the destination array.
		 */
		template <size_t size>
		inline void copy_string(const std::u16string& str, IGR_UCS2(&dest)[size])
		{
			if (str.size() >= size)
				throw std::runtime_error("String too long");

			std::copy(str.begin(), str.end(), dest);
			dest[str.size()] = 0;
		}

		/**
		 * @brief Copies a std::wstring to a destination IGR_UCS2 array.
		 *
		 * @tparam size The size of the destination array.
		 * @param str The source wide string to copy.
		 * @param dest The destination IGR_UCS2 array.
		 */
		template <size_t size>
		inline void copy_string(const std::wstring& str, IGR_UCS2(&dest)[size])
		{
			copy_string(w_to_u16(str), dest);
		}

		/**
		 * @brief Converts a wide string to an unsigned 32-bit integer, or returns a default value if conversion fails.
		 *
		 * @param s The wide string to convert.
		 * @param getDefaultValue The default value to return if conversion fails.
		 * @return The converted unsigned 32-bit integer, or the default value if conversion fails.
		 */
		uint32_t stoul_or(const std::wstring& s, uint32_t default_value);

		/**
		 * @brief Converts a wide string to a double, or returns a default value if conversion fails.
		 *
		 * This function attempts to convert the given wide string to a double. If the conversion
		 * fails, it returns the specified default value.
		 *
		 * @param s The wide string to convert.
		 * @param getDefaultValue The default value to return if conversion fails.
		 * @return The converted double, or the default value if conversion fails.
		 */
		double stod_or(const std::wstring& s, double default_value);


		/**
		 * @brief Splits a wide string into a vector of wide strings based on the given delimiters.
		 *
		 * @param str The wide string to split.
		 * @param delimiters The delimiters used to split the string.
		 * @return A vector of wide strings resulting from the split operation.
		 */
		std::vector<std::wstring> split(const std::wstring& str, const std::wstring& delimiters);

		/**
		 * @brief Throws an exception if the given return code indicates an error.
		 *
		 * @param code The return code to check.
		 * @param ecb The error control block containing error details.
		 * @param function_name The name of the function where the error occurred.
		 * @param error_message An optional error message to include in the exception.
		 * @return The original return code if no error is indicated.
		 * @throws std::runtime_error if the return code indicates an error.
		 */
		IGR_RETURN_CODE throw_on_error(IGR_RETURN_CODE code, const Error_Control_Block& ecb, const std::string& function_name, const std::string& error_message = std::string());

		/**
		 * @brief Encodes the given data into a base64 string.
		 *
		 * This function takes a pointer to the data and its length, and returns a base64-encoded string.
		 *
		 * @param data A pointer to the data to be encoded.
		 * @param length The length of the data in bytes.
		 * @return A base64-encoded string.
		 */
		std::string base64_encode(const void* data, std::size_t length);


		/**
		 * @brief A RAII wrapper for managing handle resources.
		 *
		 * This class ensures that the handle is properly released using the provided destroyer function
		 * when the handle_holder_t object goes out of scope.
		 *
		 * @tparam HandleType The getType of the handle to manage.
		 */
		template <typename HandleType>
		class handle_holder_t
		{
		private:
			typedef std::function<IGR_RETURN_CODE(HandleType, Error_Control_Block*)> DestroyerType;
			HandleType m_handle;
			DestroyerType m_destroyer;

		public:
			/**
			 * @brief Constructs a handle_holder_t with the given handle and destroyer function.
			 *
			 * @param handle The handle to manage.
			 * @param destroyer The function to call to release the handle.
			 */
			handle_holder_t(HandleType handle, const DestroyerType& destroyer)
				: m_handle(handle), m_destroyer(destroyer)
			{
			}

			// Delete copy constructor
			handle_holder_t(const handle_holder_t&) = delete;

			// Delete copy assignment operator
			handle_holder_t& operator=(const handle_holder_t&) = delete;

			/**
			 * @brief Move constructor.
			 *
			 * Transfers ownership of the handle from another handle_holder_t.
			 *
			 * @param other The other handle_holder_t to move from.
			 */
			handle_holder_t(handle_holder_t&& other)
				: m_handle(other.stealHandle())
			{
			}

			/**
			 * @brief Destructor.
			 *
			 * Ensures that the handle is properly released using the destroyer function.
			 */
			~handle_holder_t()
			{
				try
				{
					reset();
				}
				catch (...)
				{
					// Ignore exceptions in destructors
				}
			}

			/**
			 * @brief Gets the managed handle.
			 *
			 * @return The managed handle.
			 */
			HandleType getHandle() const { return m_handle; }

			/**
			 * @brief Steals the managed handle, leaving the handle_holder_t without a handle.
			 *
			 * @return The managed handle.
			 */
			HandleType stealHandle() { return std::exchange(m_handle, HandleType{}); }

			/**
			 * @brief Resets the managed handle, releasing the current handle if necessary.
			 *
			 * @param handle The new handle to manage. Defaults to 0.
			 */
			void reset(HandleType handle = 0)
			{
				if (m_handle && m_destroyer)
				{
					Error_Control_Block ecb = { 0 };
					m_destroyer(m_handle, &ecb);
				}
				m_handle = handle;
			}

			/**
			 * @brief Attaches a new handle, releasing the current handle if necessary.
			 *
			 * @return A pointer to the managed handle.
			 */
			HandleType* attach()
			{
				reset();
				return &m_handle;
			}
		};

		/**
		 * @brief A class that provides enumerable functionality for a given implementation.
		 *
		 * This class allows iteration over elements provided by the implementation.
		 *
		 * @tparam T The getType of elements to enumerate.
		 * @tparam Impl The implementation getType that provides the elements.
		 */
		template <typename T, typename Impl>
		class x_enumerable : public enumerable_t<T>
		{
		private:
			std::weak_ptr<Impl> m_impl;

			/**
			 * @brief An enumerator class for iterating over elements.
			 */
			class enumerator : public enumerable_t<T>::enumerator_t
			{
			private:
				std::weak_ptr<Impl> m_impl;
				T m_current;
				bool m_eof = false;
			public:
				/**
				 * @brief Constructs an enumerator with the given implementation.
				 *
				 * @param impl The implementation providing the elements.
				 */
				enumerator(const std::weak_ptr<Impl>& impl)
					: m_impl(impl)
					, m_current()
					, m_eof(false)
				{
				}

				/**
				 * @brief Moves to the next element in the enumeration.
				 *
				 * @return True if there is a next element, false if the end is reached.
				 */
				bool move_next() override
				{
					if (m_eof)
						return false;
					else if (!m_current)
					{
						// First item
						m_current = !m_impl.expired()
							? m_impl.lock()->getFirstChild()
							: T();
						m_eof = !m_current.ok();
						return !m_eof;
					}
					else
					{
						// Next item
						m_current = m_current.getNextSibling();
						m_eof = !m_current.ok();
						return !m_eof;
					}
				}

				/**
				 * @brief Gets the current element in the enumeration.
				 *
				 * @return The current element.
				 */
				T current() const override
				{
					return m_current;
				}
			};
		public:
			using enumerator_t = typename enumerable_t<T>::enumerator_t;

			/**
			 * @brief Constructs an x_enumerable with the given implementation.
			 *
			 * @param impl The implementation providing the elements.
			 */
			x_enumerable(const std::weak_ptr<Impl>& impl)
				: m_impl(impl)
			{
			}

			/**
			 * @brief Gets an enumerator for iterating over the elements.
			 *
			 * @return A shared pointer to the enumerator.
			 */
			std::shared_ptr< enumerator_t > get_enumerator() const override
			{
				return std::make_shared<enumerator>(m_impl);
			}
		};


		/**
		 * @brief A class that provides enumerable functionality for a standard container.
		 *
		 * This class allows iteration over elements in a standard container.
		 *
		 * @tparam T The getType of elements to enumerate.
		 * @tparam Container The container getType that provides the elements.
		 */
		template <typename T, typename Container>
		class x_std_enumerable : public enumerable_t<T>
		{
		private:
			const Container& m_container;

			/**
			 * @brief An enumerator class for iterating over elements in the container.
			 */
			class enumerator : public enumerable_t<T>::enumerator_t
			{
			private:
				const Container& m_container;
				typename Container::const_iterator m_current;
				bool m_bof = true;
				bool m_eof = false;
			public:
				/**
				 * @brief Constructs an enumerator with the given container.
				 *
				 * @param container The container providing the elements.
				 */
				enumerator(const Container& container)
					: m_container(container)
					, m_current(container.end())
				{
				}

				/**
				 * @brief Moves to the next element in the enumeration.
				 *
				 * @return True if there is a next element, false if the end is reached.
				 */
				bool move_next() override
				{
					if (m_eof)
						return false;
					else if (m_bof)
					{
						m_bof = false;
						m_current = m_container.begin();
						m_eof = m_current == m_container.end();
					}
					else
					{
						m_eof = ++m_current == m_container.end();
					}
					return !m_eof;
				}

				/**
				 * @brief Gets the current element in the enumeration.
				 *
				 * @return The current element.
				 */
				T current() const override
				{
					return *m_current;
				}
			};
		public:
			using enumerator_t = typename enumerable_t<T>::enumerator_t;

			/**
			 * @brief Constructs an x_std_enumerable with the given container.
			 *
			 * @param container The container providing the elements.
			 */
			x_std_enumerable(const Container& container)
				: m_container(container)
			{
			}

			/**
			 * @brief Gets an enumerator for iterating over the elements.
			 *
			 * @return A shared pointer to the enumerator.
			 */
			std::shared_ptr< enumerator_t > get_enumerator() const override
			{
				return std::make_shared<enumerator>(m_container);
			}
		};


		/**
		 * @brief A class that provides deep enumerable functionality for a given implementation.
		 *
		 * This class allows deep iteration over elements provided by the implementation, including nested elements.
		 *
		 * @tparam T The getType of elements to enumerate.
		 * @tparam Impl The implementation getType that provides the elements.
		 */
		template <typename T, typename Impl>
		class x_deep_enumerable : public enumerable_t<T>
		{
		private:
			std::weak_ptr<Impl> m_impl;

			/**
			 * @brief An enumerator class for iterating over elements, including nested elements.
			 */
			class enumerator : public enumerable_t<T>::enumerator_t
			{
			private:
				typedef std::tuple<T, typename enumerable_t<T>::const_iterator, typename enumerable_t<T>::const_iterator> range_t;
				std::stack<range_t> m_stack;
				T m_current;
			public:
				/**
				 * @brief Constructs an enumerator with the given implementation.
				 *
				 * @param impl The implementation providing the elements.
				 * @throws std::invalid_argument if the implementation is null.
				 */
				enumerator(const std::shared_ptr<Impl>& impl)
				{
					if (!impl)
						throw std::invalid_argument("impl");
					T root = T(impl);
					m_stack.emplace(std::make_tuple(root, root.begin(), root.end()));
				}

				/**
				 * @brief Moves to the next element in the enumeration.
				 *
				 * @return True if there is a next element, false if the end is reached.
				 */
				bool move_next() override
				{
					while (!m_stack.empty())
					{
						auto&& top = m_stack.top();
						auto&& b = std::get<1>(top);
						auto&& e = std::get<2>(top);
						if (b != e)
						{
							m_current = *b;
							// Move to the next item
							++b;

							// Push the returned item onto the stack so its children will be enumerated
							auto&& next_b = m_current.begin();
							auto&& next_e = m_current.end();
							if (next_b != next_e)
								m_stack.emplace(std::make_tuple(m_current, next_b, next_e));

							return true;
						}
						else
						{
							m_stack.pop();
						}
					}
					return false;
				}

				/**
				 * @brief Gets the current element in the enumeration.
				 *
				 * @return The current element.
				 */
				T current() const override
				{
					return m_current;
				}
			};
		public:
			using enumerator_t = typename enumerable_t<T>::enumerator_t;

			/**
			 * @brief Constructs an x_deep_enumerable with the given implementation.
			 *
			 * @param impl The implementation providing the elements.
			 */
			x_deep_enumerable(std::weak_ptr<Impl> impl)
				: m_impl(impl)
			{
			}

			/**
			 * @brief Gets an enumerator for iterating over the elements, including nested elements.
			 *
			 * @return A shared pointer to the enumerator.
			 */
			std::shared_ptr< enumerator_t > get_enumerator() const override
			{
				return std::make_shared<enumerator>(m_impl.lock());
			}
		};

		class subfile_enumerable_t : public enumerable_t<Subfile>
		{
			friend class subfile_enumerator_t;
		private:
			typedef std::function<IGR_RETURN_CODE(IGR_LONG doc, HSUBFILES* handle, Error_Control_Block* ecb)> enumerator_creator_t;
			typedef std::function<IGR_RETURN_CODE(IGR_LONG doc, const IGR_UCS2* id, struct IGR_Stream** Stream, Error_Control_Block* Error)> stream_opener_t;
			enumerator_creator_t m_creator;
			stream_opener_t m_opener;
			IGR_LONG m_doc = 0;
		public:
			subfile_enumerable_t(IGR_LONG doc, const enumerator_creator_t& creator, const stream_opener_t& opener);

			std::shared_ptr<enumerator_t> get_enumerator() const override;
		};

	} // namespace DocFilters
} // namespace Hyland
