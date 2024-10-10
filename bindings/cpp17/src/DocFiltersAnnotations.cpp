#include "DocumentFiltersObjects.h"
#include "DocFiltersCommon.h"
#include "DocFiltersAnnotations.h"

#include <iomanip>
#include <sstream>
#include <stack>
#include <type_traits>

namespace Hyland
{
	namespace DocFilters
	{
		class JsonWriter
		{
		private:
			enum class location_t
			{
				unknown,
				object,
				array,
				property
			};

			struct state_t
			{
				location_t location;
				bool first;
			};
			std::stringstream m_dest;
			std::stack<state_t> m_state;

			JsonWriter& ensure_top(location_t location)
			{
				if (m_state.empty())
					return *this;
				if (m_state.top().location != location)
					throw std::runtime_error("Invalid JSON state");
				return *this;
			}
			void ensure_property_or_array()
			{
				if (m_state.empty())
					return;
				if (m_state.top().location != location_t::array && m_state.top().location != location_t::property)
					throw std::runtime_error("Invalid JSON state");
			}
			void pop_if(location_t location)
			{
				if (m_state.empty())
					return;
				if (m_state.top().location == location)
					m_state.pop();
			}
			void start_value()
			{
				ensure_property_or_array();
				if (!m_state.empty() && m_state.top().location == location_t::array)
				{
					if (!m_state.top().first)
						m_dest << ",";
					m_state.top().first = false;
				}
			}

			void start_item()
			{
				ensure_top(location_t::object);
				if (!m_state.empty() && m_state.top().location == location_t::object)
				{
					if (!m_state.top().first)
						m_dest << ",";
					m_state.top().first = false;
				}
			}
		public:
			JsonWriter()
			{
				m_dest.imbue(std::locale::classic());
			}

			std::string str() const
			{
				return m_dest.str();
			}

			static std::string js_escape(const std::string& value)
			{
				std::string retval;
				retval.reserve(value.size());
				for (auto&& i : value)
				{
					switch (i)
					{
					case '\n':
						retval += "\\n"; break;
					case '\r':
						retval += "\\r"; break;
					case '\t':
						retval += "\\t"; break;
					case '\\':
						retval += "\\\\"; break;
					case '\"':
						retval += "\\\""; break;
					default:
						retval += i;
					}
				}
				return retval;
			}
			static std::string js_escape(const std::wstring& value)
			{
				return js_escape(w_to_u8(value));
			}
			JsonWriter& object_begin()
			{
				start_value();
				m_dest << "{";
				m_state.push({ location_t::object, true });
				return *this;
			}
			JsonWriter& object_end()
			{
				m_dest << "}";
				m_state.pop();
				pop_if(location_t::property);
				return *this;
			}
			JsonWriter& array_begin()
			{
				start_value();
				m_dest << "[";
				m_state.push({ location_t::array, true });
				return *this;
			}
			JsonWriter& array_end()
			{
				m_dest << "]";
				m_state.pop();
				pop_if(location_t::property);
				return *this;
			}
			JsonWriter& key(const std::wstring& k)
			{
				return key(w_to_u8(k));
			}
			JsonWriter& key(const std::string& k)
			{
				ensure_top(location_t::object);
				start_item();
				m_state.push({ location_t::property, true });

				m_dest << "\"" << js_escape(k) << "\":";
				return *this;
			}
			JsonWriter& value(const std::wstring& v)
			{
				start_value();
				m_dest << "\"" << js_escape(v) << "\"";
				pop_if(location_t::property);
				return *this;
			}
			JsonWriter& value(const std::string& v)
			{
				start_value();
				m_dest << "\"" << js_escape(v) << "\"";
				pop_if(location_t::property);
				return *this;
			}
			JsonWriter& value(double value)
			{
				start_value();
				m_dest << value;
				pop_if(location_t::property);
				return *this;
			}
			JsonWriter& value(int value)
			{
				start_value();
				m_dest << value;
				pop_if(location_t::property);
				return *this;
			}
			JsonWriter& value(unsigned int value)
			{
				start_value();
				m_dest << value;
				pop_if(location_t::property);
				return *this;
			}
			JsonWriter& value(bool value)
			{
				start_value();
				m_dest << (value ? "true" : "false");
				pop_if(location_t::property);
				return *this;
			}
			JsonWriter& value_null()
			{
				start_value();
				m_dest << "null";
				pop_if(location_t::property);
				return *this;
			}
			JsonWriter& value(const std::wstring& k, const std::wstring& v)
			{
				return key(k).value(v);
			}
		};

		std::optional<std::wstring> AnnoBind::get_string(const std::wstring& name, size_t max_length) const
		{
			static const size_t max_length_default = 1024;
			static const size_t size_for_text = 128*1024;

			Error_Control_Block ecb = { 0 };
			if (name == L"text")
				max_length = size_for_text;
			if (max_length == 0)
				max_length = max_length_default;

			std::wstring full_name = prefix + name;
			if (full_name.empty())
				return std::optional<std::wstring>();
			
			if (full_name.size() > 1 && full_name.back() == L'.')
				full_name.pop_back();

			auto len = static_cast<IGR_LONG>(max_length);
			std::vector<IGR_UCS2> buffer(max_length + 1);
			if (IGR_Get_Page_Annotation_Str(&anno
				, reinterpret_cast<const IGR_UCS2*>(w_to_u16(full_name).c_str())
				, &len
				, &buffer[0]
				, &ecb) == IGR_OK && len > 0)
			{
				return std::optional<std::wstring>(u16_to_w(&buffer[0], len));
			}

			return std::optional<std::wstring>();
		}

		std::optional<IGR_LONG> AnnoBind::get_long(const std::wstring& name) const
		{
			std::wstring full_name = prefix + name;
			if (full_name.empty())
				return std::optional<IGR_LONG>();
			
			if (full_name.size() > 1 && full_name.back() == L'.')
				full_name.pop_back();

			Error_Control_Block ecb = { 0 };
			IGR_LONG res = 0;
			if (IGR_Get_Page_Annotation_Long(&anno
				, reinterpret_cast<const IGR_UCS2*>(w_to_u16(full_name).c_str())
				, &res
				, &ecb) == IGR_OK)
			{
				return res;
			}
			
			return std::optional<IGR_LONG>();
		}

		IGR_LONG AnnoBind::get_long(const std::wstring& name, IGR_LONG default_value) const
		{
			return get_long(name).value_or(default_value);
		}

		AnnoBind AnnoBind::inner(const std::wstring& name) const
		{
			if (name.empty())
				return *this;
			
			return { anno, prefix + name + L"." };
		}

		/**
		 * @brief Specialization for extracting an integral value from a JSON annotation.
		 *
		 * This function extracts an integral value from the given annotation.
		 *
		 * @tparam T The getType of the integral value.
		 * @param src The source annotation.
		 * @param name The name of the annotation field.
		 * @param getDefaultValue The default value to return if the field is not found.
		 * @return The extracted integral value.
		 */
		template <typename T>
		typename std::enable_if<std::is_integral<T>::value, T>::type&
			json_bind(const AnnoBind& src, const std::wstring& name, T& dest)
		{
			// Extract the value as a long integer and cast it to the desired integral getType.
			dest = static_cast<T>(src.get_long(name, dest));
			return dest;
		}

		/**
		 * @brief Specialization for extracting a floating-point value from a JSON annotation.
		 *
		 * This function extracts a floating-point value from the given annotation.
		 *
		 * @tparam T The getType of the floating-point value.
		 * @param src The source annotation.
		 * @param name The name of the annotation field.
		 * @param dest The destination variable to store the extracted value.
		 * @return A reference to the destination variable containing the extracted floating-point value.
		 */
		template <typename T>
		typename std::enable_if<std::is_floating_point<T>::value, T>::type&
			json_bind(const AnnoBind& src, const std::wstring& name, T& dest)
		{
			// Extract the value as a double and cast it to the desired floating-point getType.
			auto v = src.get_string(name);
			if (v.has_value())
				dest = stod_or(v.value(), dest);
			return dest;
		}

		bool& json_bind(const AnnoBind& src, const std::wstring& name, bool& dest)
		{
			// Extract the value as a wide string.
			auto v = src.get_string(name);
			if (v.has_value())
				dest = v.value() == L"true";
			return dest;
		}

		/**
		 * @brief Specialization for extracting a std::wstring value from a JSON annotation.
		 *
		 * This function extracts a std::wstring value from the given annotation.
		 *
		 * @param src The source annotation.
		 * @param name The name of the annotation field.
		 * @param getDefaultValue The default value to return if the field is not found.
		 * @return The extracted std::wstring value.
		 */
		std::wstring& json_bind(const AnnoBind& src, const std::wstring& name, std::wstring& dest)
		{
			// Extract the value as a wide string.
			dest = src.get_string(name).value_or(dest);
			return dest;
		}

		/**
		 * @brief Specialization for extracting a std::string value from a JSON annotation.
		 *
		 * This function extracts a std::string value from the given annotation.
		 *
		 * @param src The source annotation.
		 * @param name The name of the annotation field.
		 * @param getDefaultValue The default value to return if the field is not found.
		 * @return The extracted std::string value.
		 */
		std::string& json_bind(const AnnoBind& src, const std::wstring& name, std::string& dest)
		{
			// Extract the value as a wide string and convert it to a UTF-8 encoded string.
			auto wstr = src.get_string(name);
			if (wstr.has_value())
				dest = w_to_u8(wstr.value());
			return dest;
		}

		// Specialization for std::vector<T>
		template <typename T>
		std::vector<T>& json_bind(const AnnoBind& src, const std::wstring& name, std::vector<T>& dest)
		{
			static const size_t max_items = 0xffff;

			auto count = src.get_long(name + L".length", 0);
			if (count > 0 && count < max_items)
			{
				dest.resize(count);
				for (auto i = 0; i < count; ++i)
				{
					AnnoBind child = src.inner(name + L"[" + std::to_wstring(i) + L"]");
					json_bind(child, L"", dest[i]);
				}
			}

			return dest;
		}

		// Specialization for std::map<K, V>
		template <typename K, typename V>
		std::map<K, V>& json_bind(const AnnoBind& /*src*/, const std::wstring& /*name*/, std::map<K, V>& dest)
		{
			return dest;
		}

		template <typename T>
		std::shared_ptr<T>& json_bind(const AnnoBind& src, const std::wstring& name, std::shared_ptr<T>& dest)
		{
			if (!dest)
				dest = std::make_shared<T>();
			json_bind(src, name, *dest);
			return dest;
		}

		template <typename T>
		std::optional<T>& json_bind(const AnnoBind& src, const std::wstring& name, std::optional<T>& dest)
		{	
			if (!dest.has_value())
				dest = T();
			json_bind(src, name, *dest);
			return dest;
		}

		Color& json_bind(const AnnoBind& src, const std::wstring& name, Color& dest)
		{
			static const size_t rgb_length = 7;
			static const size_t rgba_length = 9;

			auto val = src.get_string(name);
			if (val.has_value())
			{
				std::string s = w_to_u8(val.value());
				dest = Color();
				if (s.size() == rgb_length)
				{
					dest.a = 0xff; // NOLINT
					dest.r = static_cast<uint8_t>(std::strtoul(s.substr(1, 2).c_str(), nullptr, 16)); // NOLINT
					dest.g = static_cast<uint8_t>(std::strtoul(s.substr(3, 2).c_str(), nullptr, 16)); // NOLINT
					dest.b = static_cast<uint8_t>(std::strtoul(s.substr(5, 2).c_str(), nullptr, 16)); // NOLINT
				}
				else if (s.size() == rgba_length)
				{
					dest.a = static_cast<uint8_t>(std::strtoul(s.substr(1, 2).c_str(), nullptr, 16)); // NOLINT
					dest.r = static_cast<uint8_t>(std::strtoul(s.substr(3, 2).c_str(), nullptr, 16)); // NOLINT
					dest.g = static_cast<uint8_t>(std::strtoul(s.substr(5, 2).c_str(), nullptr, 16)); // NOLINT
					dest.b = static_cast<uint8_t>(std::strtoul(s.substr(7, 2).c_str(), nullptr, 16)); // NOLINT
				}
			}
			return dest;
		}
		
		Point& json_bind(const AnnoBind& src, const std::wstring& name, Point& dest)
		{
			AnnoBind inner = src.inner(name);
			int x = dest.x();
			int y = dest.y();
			json_bind(inner, L"x", x);
			json_bind(inner, L"y", y);
			return dest = Point(x, y);
		}

		RectUI32& json_bind(const AnnoBind& src, const std::wstring& name, RectUI32& dest)
		{
			AnnoBind inner = src.inner(name);
			json_bind(inner, L"left", dest.left);
			json_bind(inner, L"top", dest.top);
			json_bind(inner, L"right", dest.right);
			json_bind(inner, L"bottom", dest.bottom);
			return dest;
		}

		RectI32& json_bind(const AnnoBind& src, const std::wstring& name, RectI32& dest)
		{
			AnnoBind inner = src.inner(name);
			json_bind(inner, L"left", dest.left);
			json_bind(inner, L"top", dest.top);
			json_bind(inner, L"right", dest.right);
			json_bind(inner, L"bottom", dest.bottom);
			return dest;
		}

		DateTime& json_bind(const AnnoBind& src, const std::wstring& name, DateTime& dest)
		{
			dest = DateTime(src.get_string(name).value_or(std::wstring()));
			return dest;
		}

		AnnotationSerializable& json_bind(const AnnoBind& /*src*/, const std::wstring& /*name*/, AnnotationSerializable& dest)
		{
			return dest;
		}

		template <typename T>
		bool json_equal(const T& a, const T& b)
		{
			bool res = a == b;
			return res;
		}

		template <typename T>
		bool json_equal(const std::shared_ptr<T>& a, const std::shared_ptr<T>& b)
		{
			if (!a && !b)
				return true;
			if (!a || !b)
				return false;
			return *a == *b;
		}

		template <typename T>
		bool json_equal(const std::optional<T>& a, const std::optional<T>& b)
		{
			if (!a && !b)
				return true;
			if (a.has_value() && !b.has_value())
				return *a == T(); // compare the the default version
			if (!a || !b)
				return false;
			return *a == *b;
		}

		template <typename T, typename = void>
		struct has_empty_method : std::false_type {};

		template <typename T>
		struct has_empty_method<T, std::void_t<decltype(std::declval<T>().empty())>> : std::true_type {};

		template <typename T>
		bool is_empty(const T& value) {
			if constexpr (has_empty_method<T>::value) {
				return value.empty();
			}
			else {
				return false;
			}
		}

		template <typename T>
		bool is_empty(const std::shared_ptr<T>& value) {
			return !value || is_empty(*value);
		}

		template <typename T>
		bool is_empty(const std::optional<T>& value) {
			return !value.has_value() || is_empty(*value);
		}

		template <typename Enum, size_t Size>
		inline Enum string_to_enum(const std::optional<std::wstring>& value, std::pair<Enum, const char*> const (&mappings)[Size], Enum default_value, bool is_set = false)
		{
			auto compare = [](const char* a, const char* b) -> bool
				{
					if (a == nullptr || b == nullptr)
						return false;
					for (; *a && *b; ++a, ++b)
					{
						auto l = std::tolower(*a);
						auto r = std::tolower(*b);
						if (l != r)
							return false;
					}
					return *a == 0 && *b == 0;
				};

			if (!value.has_value())
				return default_value;

			auto num = stoul_or(value.value(), static_cast<uint32_t>(-1));
			if (num != static_cast<uint32_t>(-1))
				return static_cast<Enum>(num);

			std::string v = w_to_u8(value.value());

			(void) is_set;

			auto it = std::find_if(std::begin(mappings), std::end(mappings),
				[v, compare](const std::pair<Enum, const char*>& ej_pair) -> bool
				{
					return ej_pair.second && compare(v.c_str(), ej_pair.second);
				});

			return (it != std::end(mappings)
				? it
				: std::begin(mappings))->first;
		}

		template <typename Enum, size_t Size>
		JsonWriter& enum_to_json(JsonWriter& writer, Enum e, std::pair<Enum, const char*> const (&mappings)[Size], bool is_set)
		{
			if (!is_set)
			{
				auto it = std::find_if(std::begin(mappings), std::end(mappings),
					[e](const std::pair<Enum, const char*>& ej_pair) -> bool
					{
						return ej_pair.first == e;
					});
				if (it != std::end(mappings))
				{
					if (it->second == nullptr)
						writer.value_null();
					else
						writer.value(std::string(it->second));
				}
				else
					writer.value(static_cast<uint32_t>(e));
			}
			else
			{
				writer.array_begin();
				for (auto it = std::begin(mappings); it != std::end(mappings); ++it)
				{
					if (static_cast<uint32_t>(it->first) == 0)
						continue;
					
					if ((static_cast<uint32_t>(e) & static_cast<uint32_t>(it->first)) == static_cast<uint32_t>(it->first))
					{
						if (it->second != nullptr)
							writer.value(std::string(it->second));
					}
				}
				writer.array_end();
			}
			return writer;
		}

		template <typename T>
		JsonWriter& json_write(JsonWriter& writer, const T& value)
		{
			return writer.value(value);
		}

		template <typename T>
		JsonWriter& json_write(JsonWriter& writer, const std::vector<T>& src)
		{
			writer.array_begin();
			for (auto&& item : src)
				json_write(writer, item);
			writer.array_end();
			return writer;
		}

		template <typename K, typename V>
		JsonWriter& json_write(JsonWriter& writer, const std::map<K, V>& src)
		{
			writer.object_begin();
			for (auto&& item : src)
			{
				writer.key(item.first);
				json_write(writer, item.second);
			}
			writer.object_end();
			return writer;
		}

		template <typename T>
		JsonWriter& json_write(JsonWriter& writer, const std::shared_ptr<T>& src)
		{
			if (src)
				json_write(writer, *src);
			else
				writer.value_null();
			return writer;
		}

		template <typename T>
		JsonWriter& json_write(JsonWriter& writer, const std::optional<T>& src)
		{
			if (src.has_value())
				json_write(writer, *src);
			else
				writer.value_null();
			return writer;
		}

		JsonWriter& json_write_props(JsonWriter& writer, const AnnotationSerializable& /*src*/)
		{
			return writer;
		}
		
		JsonWriter& json_write(JsonWriter& writer, const AnnotationSerializable& /*src*/)
		{
			return writer;
		}

		template <> JsonWriter& json_write<Color>(JsonWriter& writer, const Color& value)
		{
			if (value.r == 0 && value.g == 0 && value.b == 0 && value.a == 0)
				writer.value_null();
			else
			{
				std::stringstream ss;
				ss.imbue(std::locale::classic());
				ss << "#" << std::hex << std::setfill('0');
				if (value.a != 0xff) // NOLINT
					ss << std::setw(2) << static_cast<int>(value.a);
				ss << std::setw(2) << static_cast<int>(value.r)
					<< std::setw(2) << static_cast<int>(value.g)
					<< std::setw(2) << static_cast<int>(value.b);
				writer.value(ss.str());
			}
			return writer;
		}

		template <> JsonWriter& json_write<Point>(JsonWriter& writer, const Point& value)
		{
			return writer.object_begin()
				.key("x").value(value.x())
				.key("y").value(value.y())
				.object_end();
		}

		template <> JsonWriter& json_write<RectUI32>(JsonWriter& writer, const RectUI32& value)
		{
			return writer.object_begin()
				.key("left").value(static_cast<uint32_t>(value.left))
				.key("top").value(static_cast<uint32_t>(value.top))
				.key("right").value(static_cast<uint32_t>(value.right))
				.key("bottom").value(static_cast<uint32_t>(value.bottom))
				.object_end();
		}

		template <> JsonWriter& json_write<RectI32>(JsonWriter& writer, const RectI32& value)
		{
			return writer.object_begin()
				.key("left").value(value.left)
				.key("top").value(value.top)
				.key("right").value(value.right)
				.key("bottom").value(value.bottom)
				.object_end();
		}

		template <> JsonWriter& json_write<DateTime>(JsonWriter& writer, const DateTime& value)
		{
			return writer.value(value.ToIsoString());
		}

#ifdef _MSC_VER
#define WSTR(S) L##S
#else
#define WSTR(S) L ## S
#endif


#define DOCFILTERS_ENUM_SERIALIZABLE_SET(ENUM_TYPE, ...) \
	inline ENUM_TYPE& json_bind(const AnnoBind& src, const std::wstring& name, ENUM_TYPE& e) { \
		static const std::pair<ENUM_TYPE, const char*> m[] = __VA_ARGS__;                         \
		return e = string_to_enum<ENUM_TYPE>(src.get_string(name), m, e, true) ;                  \
	}                                                                                             \
	inline JsonWriter& json_write(JsonWriter& writer, ENUM_TYPE e) {                            \
		static const std::pair<ENUM_TYPE, const char*> m[] = __VA_ARGS__;                         \
		return enum_to_json(writer, e, m, true);                                                  \
	}

#define DOCFILTERS_ENUM_SERIALIZABLE(ENUM_TYPE, ...)  \
	inline ENUM_TYPE& json_bind(const AnnoBind& src, const std::wstring& name, ENUM_TYPE& e) { \
		static const std::pair<ENUM_TYPE, const char*> m[] = __VA_ARGS__;                         \
		return e = string_to_enum<ENUM_TYPE>(src.get_string(name), m, e, false);                  \
	}                                                                                             \
	inline JsonWriter& json_write(JsonWriter& writer, ENUM_TYPE e) {                            \
		static const std::pair<ENUM_TYPE, const char*> m[] = __VA_ARGS__;                         \
		return enum_to_json(writer, e, m, false);                                                 \
	}

#define DOCFILTERS_JSON_TO(prop) if (!is_empty(src.prop)) { writer.key(#prop); json_write(writer, src.prop); }
#define DOCFILTERS_JSON_FROM(prop) json_bind(obj, WSTR(#prop), dest.prop);
#define DOCFILTERS_JSON_COMPARE(prop) result &= json_equal(prop, other.prop);

#define DOCFILTERS_SERIALIZABLE_OBJECT(Type, Base, ...)  \
    Type& json_bind(const AnnoBind& src, const std::wstring& name, Type& dest) { \
		json_bind(src, name, static_cast<Base&>(dest)); \
		AnnoBind obj = src.inner(name); \
		DOCFILTERS_JSON_EXPAND(DOCFILTERS_JSON_PASTE(DOCFILTERS_JSON_FROM, __VA_ARGS__)) return dest; } \
	JsonWriter& json_write_props(JsonWriter& writer, const Type& src) { \
		json_write_props(writer, static_cast<const Base&>(src)); \
		DOCFILTERS_JSON_EXPAND(DOCFILTERS_JSON_PASTE(DOCFILTERS_JSON_TO, __VA_ARGS__)) \
		return writer; } \
	JsonWriter& json_write(JsonWriter& writer, const Type& src) { \
		writer.object_begin(); \
		json_write_props(writer, src); \
		writer.object_end();\
		return writer; } \
	std::string Type::serialize() const { \
		JsonWriter writer; \
		json_write(writer, *this); \
		return writer.str(); } \
	bool Type::operator==(const Type& other) const { \
		bool result = Base::operator==(static_cast<const Base&>(other)); \
		DOCFILTERS_JSON_EXPAND(DOCFILTERS_JSON_PASTE(DOCFILTERS_JSON_COMPARE, __VA_ARGS__)) return result; } \

#define DOCFILTERS_SERIALIZABLE_ANNOTATION(Type, Base, ...) \
	DOCFILTERS_SERIALIZABLE_OBJECT(Type, Base, __VA_ARGS__) \
	void Type::bind(const IGR_Annotation& anno) { json_bind({anno}, std::wstring(), *this); } 

#define DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(Type, Base)  \
    Type& json_bind(const AnnoBind& src, const std::wstring& name, Type& dest) { \
		json_bind(src, name, static_cast<Base&>(dest)); \
		return dest; } \
	JsonWriter& json_write_props(JsonWriter& writer, const Type& src) { \
		json_write_props(writer, static_cast<const Base&>(src)); \
		return writer; } \
	JsonWriter& json_write(JsonWriter& writer, const Type& src) { \
		writer.object_begin(); \
		json_write_props(writer, static_cast<const Base&>(src)); \
		writer.object_end();\
		return writer; } \
	std::string Type::serialize() const { \
		JsonWriter writer; \
		json_write(writer, *this); \
		return writer.str(); } \
	void Type::bind(const IGR_Annotation& anno) { json_bind({anno}, std::wstring(), *this); } \
	bool Type::operator==(const Type& other) const { \
		return Base::operator==(static_cast<const Base&>(other)); }


		DOCFILTERS_ENUM_SERIALIZABLE_SET(AnnotationBase::Flags, {
			{ AnnotationBase::Flags::None, "none" },
			{ AnnotationBase::Flags::Invisible, "invisible" },
			{ AnnotationBase::Flags::Hidden, "hidden" },
			{ AnnotationBase::Flags::Print, "print" },
			{ AnnotationBase::Flags::NoZoom, "noZoom" },
			{ AnnotationBase::Flags::NoRotate, "noRotate" },
			{ AnnotationBase::Flags::NoView, "noView" },
			{ AnnotationBase::Flags::ReadOnly, "readOnly" },
			{ AnnotationBase::Flags::Locked, "locked" },
			{ AnnotationBase::Flags::ToogleNoView, "toggleNoView" },
			{ AnnotationBase::Flags::LockedContents, "lockedContents" },
			});
		DOCFILTERS_ENUM_SERIALIZABLE(AnnotationBase::Type, {
			{ AnnotationBase::Type::Unknown, "unknown" },
			{ AnnotationBase::Type::Text, "text" },
			{ AnnotationBase::Type::Link, "link" },
			{ AnnotationBase::Type::Freetext, "freetext" },
			{ AnnotationBase::Type::Line, "line" },
			{ AnnotationBase::Type::Rectangle, "rectangle" },
			{ AnnotationBase::Type::Ellipse, "ellipse" },
			{ AnnotationBase::Type::Polygon, "polygon" },
			{ AnnotationBase::Type::Polyline, "polyline" },
			{ AnnotationBase::Type::Highlight, "highlight" },
			{ AnnotationBase::Type::Underline, "underline" },
			{ AnnotationBase::Type::Squiggly, "squiggly" },
			{ AnnotationBase::Type::Strikeout, "strikeout" },
			{ AnnotationBase::Type::Stamp, "stamp" },
			{ AnnotationBase::Type::Caret, "caret" },
			{ AnnotationBase::Type::Ink, "ink" },
			{ AnnotationBase::Type::Popup, "popup" },
			{ AnnotationBase::Type::FileAttachment, "fileAttachment" },
			{ AnnotationBase::Type::Sound, "sound" },
			{ AnnotationBase::Type::Movie, "movie" },
			{ AnnotationBase::Type::Widget, "widget" },
			{ AnnotationBase::Type::Screen, "screen" },
			{ AnnotationBase::Type::Printermark, "printermark" },
			{ AnnotationBase::Type::Trapnet, "trapnet" },
			{ AnnotationBase::Type::Watermark, "watermark" },
			{ AnnotationBase::Type::ThreeD, "three_d" },
			{ AnnotationBase::Type::Barcode, "barcode" },
			{ AnnotationBase::Type::NamedDestination, "namedDestination" },
			{ AnnotationBase::Type::Rectangle, "square" },
			{ AnnotationBase::Type::Ellipse, "circle" },
			});
		DOCFILTERS_ENUM_SERIALIZABLE(AnnotationBase::BorderStyleType, {
			{AnnotationBase::BorderStyleType::Unknown, "unknown"},
			{AnnotationBase::BorderStyleType::Solid, "solid"},
			{AnnotationBase::BorderStyleType::Dashed, "dashed"},
			{AnnotationBase::BorderStyleType::Beveled, "beveled"},
			{AnnotationBase::BorderStyleType::Inset, "inset"},
			{AnnotationBase::BorderStyleType::Underline, "underline"},
			{AnnotationBase::BorderStyleType::RightOpenArrow, "right_open_arrow"},
			{AnnotationBase::BorderStyleType::RightClosedArrow, "right_closed_arrow"},
			{AnnotationBase::BorderStyleType::Slash, "slash"}
			});
		DOCFILTERS_ENUM_SERIALIZABLE(AnnotationBase::LineEndingType, {
			{AnnotationBase::LineEndingType::None, "none"},
			{AnnotationBase::LineEndingType::Square, "square"},
			{AnnotationBase::LineEndingType::Circle, "circle"},
			{AnnotationBase::LineEndingType::Diamond, "diamond"},
			{AnnotationBase::LineEndingType::OpenArrow, "openArrow"},
			{AnnotationBase::LineEndingType::ClosedArrow, "closedArrow"},
			{AnnotationBase::LineEndingType::Butt, "butt"},
			{AnnotationBase::LineEndingType::RightOpenArrow, "rightOpenArrow"},
			{AnnotationBase::LineEndingType::RightClosedArrow, "rightClosedArrow"},
			{AnnotationBase::LineEndingType::Slash, "slash"}
			});
		DOCFILTERS_ENUM_SERIALIZABLE(AnnotationBase::AlignmentType, {
			{AnnotationBase::AlignmentType::Left, "left"},
			{AnnotationBase::AlignmentType::Center, "center"},
			{AnnotationBase::AlignmentType::Right, "right"}
			});
		DOCFILTERS_ENUM_SERIALIZABLE(AnnotationLink::HighlightType, {
			{ AnnotationLink::HighlightType::None, "none" },
			{ AnnotationLink::HighlightType::Invert, "invert" },
			{ AnnotationLink::HighlightType::Outline, "outline" },
			{ AnnotationLink::HighlightType::Push, "push" }
			});
		DOCFILTERS_ENUM_SERIALIZABLE(AnnotationAction::ActionType, {
			{ AnnotationAction::ActionType::Unknown, nullptr },
			{ AnnotationAction::ActionType::GoTo, "GoTo" },
			{ AnnotationAction::ActionType::Uri, "URI" },
			{ AnnotationAction::ActionType::GoToRemote, "GoToR" },
			{ AnnotationAction::ActionType::Named, "Named"}
			});

		DOCFILTERS_SERIALIZABLE_OBJECT(AnnotationBase::AppearanceStream, AnnotationSerializable, content, contentType, encoding);
		DOCFILTERS_SERIALIZABLE_OBJECT(AnnotationBase::AppearanceStreams, AnnotationSerializable, normal, rollover, down);
		DOCFILTERS_SERIALIZABLE_OBJECT(AnnotationBase::LineEndings, AnnotationSerializable, begin, end);
		DOCFILTERS_SERIALIZABLE_OBJECT(AnnotationBase::BorderStyle, AnnotationSerializable, type, width, intensity, dash);
		DOCFILTERS_SERIALIZABLE_OBJECT(AnnotationBase::DefaultAppearance, AnnotationSerializable, fontName, fontSize, textColor);
		DOCFILTERS_SERIALIZABLE_OBJECT(AnnotationBase, AnnotationSerializable, type, flags, name, dateModified, border, color, rect, text);

		DOCFILTERS_SERIALIZABLE_OBJECT(AnnotationAction, AnnotationSerializable, newWindow, type, page, uri, name, zoom, rect, filename);
		DOCFILTERS_SERIALIZABLE_OBJECT(AnnotationNote, AnnotationBase, state, stateModel, author);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(Annotation, AnnotationBase, subject, title, intent, opacity, dateCreated, defaultAppearance, popup, replies, appearance, points);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationPopup, AnnotationBase, open);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationStickyNote, Annotation, iconName, open, author, state, stateModel);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationLink, Annotation, highlight, action);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationFreeText, Annotation, defaultAppearance, alignment);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationLine, Annotation, lineEndings, interiorColor);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationRectangle, Annotation, lineEndings, interiorColor, rectDifferences);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationEllipse, Annotation, interiorColor, rectDifferences);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationPolygon, Annotation, interiorColor);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationPolyline, Annotation, lineEndings);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationStamp, Annotation, defaultAppearance);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationInk, Annotation, inkList);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationBarcode, Annotation, subType, bgColor, caption, content, errorCorrectionLevel, margin);
		DOCFILTERS_SERIALIZABLE_ANNOTATION(AnnotationGS1_128, AnnotationBarcode, autoCaption, parts);

		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationMarkup, Annotation);
		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationHighlight, AnnotationMarkup);
		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationSquiggly, AnnotationMarkup);
		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationStrikeOut, AnnotationMarkup);
		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationUnderline, AnnotationMarkup);
		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationQrCode, AnnotationBarcode);
		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationAztec, AnnotationBarcode);
		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationDatamatrix, AnnotationBarcode);
		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationPDF417, AnnotationBarcode);
		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationCode39, AnnotationBarcode);
		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationCode93, AnnotationBarcode);
		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationCode128, AnnotationBarcode);
		DOCFILTERS_SERIALIZABLE_ANNOTATION_EMPTY(AnnotationNamedDestination, Annotation);

		// ------------------------------------------------------------

		std::unique_ptr<Annotation> Annotation::make(const IGR_Annotation& anno)
		{
			auto create = [anno]() -> std::unique_ptr<Annotation> {
				switch (anno.type)
				{
				case IGR_ANNOTATION_TEXT:
					return std::make_unique<AnnotationStickyNote>();
				case IGR_ANNOTATION_POPUP:
					return std::make_unique<AnnotationPopup>();
				case IGR_ANNOTATION_LINK:
					return std::make_unique<AnnotationLink>();
				case IGR_ANNOTATION_FREETEXT:
					return std::make_unique<AnnotationFreeText>();
				case IGR_ANNOTATION_LINE:
					return std::make_unique<AnnotationLine>();
				case IGR_ANNOTATION_SQUARE:
					return std::make_unique<AnnotationRectangle>();
				case IGR_ANNOTATION_CIRCLE:
					return std::make_unique<AnnotationEllipse>();
				case IGR_ANNOTATION_POLYGON:
					return std::make_unique<AnnotationPolygon>();
				case IGR_ANNOTATION_POLYLINE:
					return std::make_unique<AnnotationPolyline>();
				case IGR_ANNOTATION_HIGHLIGHT:
					return std::make_unique<AnnotationHighlight>();
				case IGR_ANNOTATION_UNDERLINE:
					return std::make_unique<AnnotationUnderline>();
				case IGR_ANNOTATION_SQUIGGLE:
					return std::make_unique<AnnotationSquiggly>();
				case IGR_ANNOTATION_STRIKEOUT:
					return std::make_unique<AnnotationStrikeOut>();
				case IGR_ANNOTATION_STAMP:
					return std::make_unique<AnnotationStamp>();
				case IGR_ANNOTATION_INK:
					return std::make_unique<AnnotationInk>();
				case IGR_ANNOTATION_BARCODE:
					return std::make_unique<AnnotationBarcode>();
				case IGR_ANNOTATION_NAMED_DESTINATION:
					return std::make_unique<AnnotationNamedDestination>();

				// No mappings...				
				case IGR_ANNOTATION_CARET:
				case IGR_ANNOTATION_FILEATTACHMENT:
				case IGR_ANNOTATION_SOUND:
				case IGR_ANNOTATION_MOVIE:
				case IGR_ANNOTATION_WIDGET:
				case IGR_ANNOTATION_SCREEN:
				case IGR_ANNOTATION_PRINTERMARK:
				case IGR_ANNOTATION_TRAPNET:
				case IGR_ANNOTATION_WATERMARK:
				case IGR_ANNOTATION_3D:
				case IGR_ANNOTATION_UNKNOWN:
				default:
					return std::make_unique<Annotation>();
				}};
			auto res = create();
			res->bind(anno);
			return res;
		}

		bool Annotation::is_equal(const Annotation& other) const
		{
			return Annotation::is_equal(*this, other);
		}

		bool Annotation::is_equal(const Annotation& a, const Annotation& b)
		{
			if (&a == &b)
				return true;
			if (a.type != b.type)
				return false;

			switch (a.type)
			{
			case AnnotationPopup::AnnoType:
				return dynamic_cast<const AnnotationPopup&>(a) == dynamic_cast<const AnnotationPopup&>(b);
			case AnnotationLink::AnnoType:
				return dynamic_cast<const AnnotationLink&>(a) == dynamic_cast<const AnnotationLink&>(b);
			case AnnotationStickyNote::AnnoType:
				return dynamic_cast<const AnnotationStickyNote&>(a) == dynamic_cast<const AnnotationStickyNote&>(b);
			case AnnotationFreeText::AnnoType:
				return dynamic_cast<const AnnotationFreeText&>(a) == dynamic_cast<const AnnotationFreeText&>(b);
			case AnnotationLine::AnnoType:
				return dynamic_cast<const AnnotationLine&>(a) == dynamic_cast<const AnnotationLine&>(b);
			case AnnotationRectangle::AnnoType:
				return dynamic_cast<const AnnotationRectangle&>(a) == dynamic_cast<const AnnotationRectangle&>(b);
			case AnnotationEllipse::AnnoType:
				return dynamic_cast<const AnnotationEllipse&>(a) == dynamic_cast<const AnnotationEllipse&>(b);
			case AnnotationPolygon::AnnoType:
				return dynamic_cast<const AnnotationPolygon&>(a) == dynamic_cast<const AnnotationPolygon&>(b);
			case AnnotationPolyline::AnnoType:
				return dynamic_cast<const AnnotationPolyline&>(a) == dynamic_cast<const AnnotationPolyline&>(b);
			case AnnotationHighlight::AnnoType:
				return dynamic_cast<const AnnotationHighlight&>(a) == dynamic_cast<const AnnotationHighlight&>(b);
			case AnnotationUnderline::AnnoType:
				return dynamic_cast<const AnnotationUnderline&>(a) == dynamic_cast<const AnnotationUnderline&>(b);
			case AnnotationSquiggly::AnnoType:
				return dynamic_cast<const AnnotationSquiggly&>(a) == dynamic_cast<const AnnotationSquiggly&>(b);
			case AnnotationStrikeOut::AnnoType:
				return dynamic_cast<const AnnotationStrikeOut&>(a) == dynamic_cast<const AnnotationStrikeOut&>(b);
			case AnnotationStamp::AnnoType:
				return dynamic_cast<const AnnotationStamp&>(a) == dynamic_cast<const AnnotationStamp&>(b);
			case AnnotationInk::AnnoType:
				return dynamic_cast<const AnnotationInk&>(a) == dynamic_cast<const AnnotationInk&>(b);
			case AnnotationBarcode::AnnoType:
				return dynamic_cast<const AnnotationBarcode&>(a) == dynamic_cast<const AnnotationBarcode&>(b);
			case AnnotationNamedDestination::AnnoType:
				return dynamic_cast<const AnnotationNamedDestination&>(a) == dynamic_cast<const AnnotationNamedDestination&>(b);
			default:
				return dynamic_cast<const Annotation&>(a) == dynamic_cast<const Annotation&>(b);
			}

		}
	} // namespace DocFilters
} // namespace Hyland
