#ifndef INCLUDE_JSON_H
#define INCLUDE_JSON_H

#include <vector>
#include <string>
#include <map>
#include <stack>
#include <deque>
#include <variant>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <functional>
#include <cassert>
#include <sstream>
#include <fstream>
#include <chrono>

namespace Json
{
	// Data Forehead Definition
	class Element;
	class Value;
	class Object;
	class Array;
	class Exception;

	//  Base Type Definition
	using ValueType = std::variant <
		std::nullptr_t,
		bool,
		int,
		float,
		std::string
	>;

	using String = std::string;

	template<typename K, typename V>
	using Map = std::map<K, V>;
	
	// Exception Class
	class Exception : public std::runtime_error
	{
	public:
		explicit Exception(const std::string& message) :std::runtime_error(message) {}
	};

	// Abstract json class, from which every element in json data extends.
	// Example1: null
	// Example2: true
	// Example3: 1
	// Example4: 3.14
	// Example5: "Example String"
	// Example6: [value1, value2, value3, ...]
	// Example7: {"key1": value1, "key2": value2, ...}
	class Element
	{
	public:
		virtual ~Element() {}

		// Class Type Check
		constexpr virtual bool is_primitive() const noexcept { return false; }
		constexpr virtual bool is_array() const noexcept { return false; }
		constexpr virtual bool is_object() const noexcept { return false; }

		// Class Type Transformation
		virtual Value* as_primitive() { throw Exception("Type Error!"); }
		virtual Array* as_array() { throw Exception("Type Error!"); }
		virtual Object* as_object() { throw Exception("Type Error!"); }

		// Implement
		virtual void clear() {}
		virtual Element* copy() const noexcept = 0;

		// Serialization
		virtual std::string serialize() const noexcept { return ""; }

		// Comparison
		virtual bool operator==(const Element& other) const noexcept { return false; }
		virtual bool operator!=(const Element& other) const noexcept { return true; }
	};

	// json value, which is others' children node.
	// Example1: null
	// Example2: true
	// Example3: 1
	// Example4: 3.14
	// Example5: "Example String"
	class Value : public Element
	{
	private:
		ValueType value;
	public:
		// Construction
		Value() :value(nullptr) {}
		Value(const bool& val) :value(val) {}
		Value(const int& val) :value(val) {}
		Value(const float& val) :value(val) {}
		Value(const std::string& val) :value(val) {}
		Value(const char* val) :value(val) {}

		Value(const Value& val) :value(val.value) {}
		Value(const Value* val) :value(val->value) {}

		~Value() override = default;

		// Class Type Check
		constexpr bool is_primitive() const noexcept override { return true; }

		// Class Type Transformation
		Value* as_primitive() override { return this; }

		// Value Type Check
		template<typename T>
		constexpr bool hasT() const noexcept { return std::holds_alternative<T>(value); }
		constexpr bool has_null() const noexcept { return hasT<std::nullptr_t>(); }
		constexpr bool has_bool() const noexcept { return hasT<bool>(); }
		constexpr bool has_int() const noexcept { return hasT<int>(); }
		constexpr bool has_float() const noexcept { return hasT<float>(); }
		constexpr bool has_str() const noexcept { return hasT<std::string>(); }

		// Value Getter
		constexpr ValueType get_value() { return value; }
		template<typename T>
		constexpr T as_T() const { return std::get<T>(value); }
		constexpr bool as_bool() const { if (has_bool()) return as_T<bool>(); else throw Exception("Not bool type!"); }
		constexpr int as_int() const { if (has_int()) return as_T<int>(); else if (has_float()) return (int) as_T<float>(); else throw Exception("Not int type!"); }
		constexpr float as_float() const { if (has_float()) return as_T<float>(); else throw Exception("Not float type!"); }
		constexpr std::string as_str() const { if (has_str()) return as_T<std::string>(); else throw Exception("Not string type!"); }

		// Implement
		Value* copy() const noexcept override { return new Value(this); }

		// Serialization
		constexpr std::string serialize() const noexcept override
		{
			if (has_int()) return std::to_string(as_int());
			else if (has_float()) return std::to_string(as_float());
			else if (has_str()) return "\"" + as_str() + "\"";
			else if (has_bool()) return as_bool() ? "true" : "false";
			else if (has_null()) return "null";
			else return "";
		}

		// Comparison
		bool operator==(Value& other) const noexcept { return value == other.value; }
		bool operator!=(Value& other) const noexcept { return value != other.value; }
		bool operator==(const Element& other) const noexcept override
		{
			if (auto* other_cast = dynamic_cast<const Value*>(&other)) { return this == other_cast; }
			else return false;
		}
		bool operator!=(const Element& other) const noexcept override { return !((*this) == other); }
	};

	// json array, which contains a series of elements.
	// Example: [value1, value2, value3, ...]
	class Array : public Element
	{
	private:
		std::vector<Element*> m_arr;
	public:
		// Construction
		Array() :m_arr() {}
		Array(const size_t count) :m_arr(count) {}
		Array(const std::vector<Element*>& val) noexcept { append_all_ptr(val); }
		Array(const Array& val) noexcept { append_all(val.as_vector()); }
		Array(const Array* val) noexcept { append_all(val->as_vector()); }
		Array(Array&& other) noexcept: m_arr(std::move(other.m_arr)) {}
		~Array() override { clear(); }

		// Class Type Check
		constexpr virtual bool is_array() const noexcept override { return true; }

		// Class Type Transformation
		Array* as_array() override { return this; }
		Element* as_element() const noexcept { return size() == 1 ? m_arr[0] : nullptr; }

		// Value Getter
		constexpr std::vector<Element*> as_vector() const noexcept { return m_arr; }

		// Implement
		void clear() override
		{
			// Every element belongs to their parent element, so if a element is cleared, its children are supposed not to exist.
			for (const auto& it : m_arr) if (it != nullptr) it->clear(), delete it;
			m_arr.clear();
		}
		Array* copy()  const noexcept override { return new Array(this); }

		// Container Operation
		size_t size() const noexcept { return m_arr.size(); }
		bool empty() const noexcept { return m_arr.empty(); }
		bool contains(Element* child) { return std::find(m_arr.begin(), m_arr.end(), child) != m_arr.end(); }

		// Element Getter (only get the pointer, remember not to delete!)
		Element* get_ptr(size_t idx) noexcept { return idx < size() ? m_arr[idx] : nullptr; }
		const Element* get_ptr(size_t idx) const noexcept { return idx < size() ? m_arr[idx] : nullptr; }
		Element* operator[](size_t idx) noexcept { return get_ptr(idx); }
		const Element* operator[](size_t idx) const noexcept { return get_ptr(idx); }

		// Element Getter (copy a new element, remember to delete!)
		Element* get(size_t idx) noexcept { return get_ptr(idx)->copy(); }

		// Element Addition (only add the pointer, remember not to delete!)
		constexpr void append_ptr(Element* child) noexcept { m_arr.push_back(child); }
		constexpr void append_all_ptr(const std::vector<Element*>& children) noexcept { for (auto& child : children) append_ptr(child); }

		// Element Addition (copy the original element)
		constexpr void append(const Element& child) noexcept { append_ptr(child.copy()); }
		constexpr void append(const Element* child) noexcept { append_ptr(child->copy()); }
		constexpr void append_all(const std::vector<Element>& children) noexcept { for (auto& child : children) append(child); }
		constexpr void append_all(const std::vector<Element*>& children) noexcept { for (auto& child : children) append(child); }
		void set(size_t idx, Element* child)
		{
			if (size() <= idx) m_arr.resize(idx);
			// Every element belongs to their parent element, so if a element is replaced, it's supposed not to exist.
			if (m_arr[idx] != nullptr) delete m_arr[idx];
			m_arr[idx] = child;
		}

		// Value Addition (use the value to create a new element)
		constexpr void append(const int&);
		constexpr void append(const float&);
		constexpr void append(const bool&);
		constexpr void append(const std::string&);
		constexpr void append(const char*);
		constexpr void append(const std::vector<Element*>&);
		constexpr void append(const Map<std::string, Element*>&);

		void erase(const size_t& idx) { m_arr.erase(m_arr.begin() + idx); }
		void remove(const Element* child)
		{
			auto it = std::find(m_arr.begin(), m_arr.end(), child);
			if (it != m_arr.end()) m_arr.erase(it);
		}
		void remove_all(const std::vector<Element*>& children) { for (const Element* child : children) remove(child); }

		// Serializaion
		std::string serialize() const noexcept override {
			std::string _str;
			_str.push_back('[');
			bool is_first = true;
			for (const auto& it : m_arr)
			{
				_str += it->serialize();
				if (is_first) is_first = false;
				else _str.push_back(',');
			}
			_str.push_back(']');
			return _str;
		}

		// Comparing all the elements, which might spends tons of time. Use it carefully!
		bool operator==(const Array& other) const noexcept { return this->m_arr == other.m_arr; }
		bool operator!=(const Array& other) const noexcept { return this->m_arr != other.m_arr; }
		bool operator==(const Element& other) const noexcept override 
		{
			if (auto* other_cast = dynamic_cast<const Array*>(&other)) return (*this) == (*other_cast);
			else return false;
		}
		bool operator!=(const Element& other) const noexcept override { return !((*this) == other); }
	};

	// Json object, which contains many pairs of key and value.
	// example: {"key1": value1, "key2": value2, ...}
	class Object :public Element
	{
	private:
		Map<std::string, Element*> m_map;
	public:
		// Construction
		Object() { m_map.clear(); };
		Object(const Map<std::string, Element*>& val) noexcept { insert_all_ptr(val); }
		Object(const Object& other) noexcept { insert_all(other.m_map); }
		Object(const Object* other) noexcept { insert_all(other->m_map); }
		Object(Object&& other) noexcept : m_map(std::move(other.m_map)) {}
		Object(const Array& other);
		Object(const Array* other);
		~Object() override { clear(); }

		// Class Type Check
		constexpr bool is_object() const noexcept override { return true; }

		// Class Type Transformation
		Object* as_object() override { return this; }

		// Value Getter (get the pointer map, remember not to delete!)
		Map<std::string, Element*> as_ptr_map() const noexcept { return m_map; }

		// Implement
		void clear() override
		{
			// Every element belongs to their parent element, so if a element is cleared, its children are supposed not to exist.
			for (auto it = m_map.begin(); it != m_map.end(); ++it) (it->second)->clear(), delete it->second;
			m_map.clear();
		}
		Object* copy()  const noexcept override { return new Object(this); }

		// Container Operation
		size_t size() const noexcept { return m_map.size(); }
		bool empty() const noexcept { return m_map.empty(); }
		bool contains(const std::string& key) const noexcept { return m_map.contains(key); }
		bool contains(const char* key) const noexcept { return m_map.contains(key); }

		// Element Getter (only get the pointer, remember not to delete!)
		Element* get_ptr(const std::string& key) noexcept { return m_map[key]; }
		Element* get_ptr(const char* key) noexcept { return m_map[key]; }
		Element* operator[](const std::string& key) noexcept { return get_ptr(key); }
		Element* operator[](const char* key) noexcept { return get_ptr(key); }

		// Element Getter (copy a new element, remember to delete!)
		Element* get(const std::string& key) noexcept { return contains(key) ? get_ptr(key)->copy() : nullptr; }
		Element* get(const char* key) noexcept { return contains(key) ? get_ptr(key)->copy() : nullptr; }

		// Element Addition (only add the pointer, remember not to delete!)
		void insert_ptr(const std::string& property, Element* child) noexcept
		{
			// Every element belongs to their parent element, so if a element is replaced, it's supposed not to exist.
			if (m_map[property] != nullptr) delete m_map[property];
			m_map[property] = child;
		}
		void insert_ptr(const char* property, Element* child) noexcept
		{
			// Every element belongs to their parent element, so if a element is replaced, it's supposed not to exist.
			if (m_map[property] != nullptr) delete m_map[property];
			m_map[property] = child;
		}
		void insert_all_ptr(const Map<std::string, Element*>& other) noexcept { for (auto& child : other) insert_ptr(child.first, child.second); }

		// Element Addition (copy the original element)
		void insert(const std::string& property, const Element& child) noexcept { insert_ptr(property, child.copy()); }
		void insert(const std::string& property, const Element* child) noexcept { insert_ptr(property, child->copy()); }
		void insert(const char* property, const Element& child) noexcept { insert_ptr(property, child.copy()); }
		void insert(const char* property, const Element* child) noexcept { insert_ptr(property, child->copy()); }
		void insert_all(const Map<std::string, Element*>& other) noexcept { for (const auto& child : other) insert(child.first, *(child.second)); }

		// Value Addition (use the original value to create new element)
		void insert(const std::string&, const int&) noexcept;
		void insert(const std::string&, const float&) noexcept;
		void insert(const std::string&, const bool&) noexcept;
		void insert(const std::string&, const std::string&) noexcept;
		void insert(const std::string&, const std::vector<Element*>&) noexcept;
		void insert(const std::string&, const Map<std::string, Element*>&) noexcept;
		void insert(const char*, const int&) noexcept;
		void insert(const char*, const float&) noexcept;
		void insert(const char*, const bool&) noexcept;
		void insert(const char*, const std::string&) noexcept;
		void insert(const char*, const std::vector<Element*>&) noexcept;
		void insert(const char*, const Map<std::string, Element*>&) noexcept;

		// Serialization
		std::string serialize() const noexcept override
		{
			std::string _str;
			_str.push_back('{');
			bool is_first = true;
			for (const auto& [k, v]: m_map)
			{
				_str.push_back('\"');
				_str += k;
				_str.push_back('\"');
				_str.push_back(':');
				_str += v->serialize();
				if (is_first) is_first = false;
				else _str.push_back(',');
			}
			_str.push_back('}');
			return _str;
		}

		// Comparing all the pairs, which might spends tons of time. Use it carefully!
		bool operator==(Object& other) const noexcept
		{
			if (size() != other.size()) return false;
			for (const auto& [k, v] : m_map)
			{
				if (!other.m_map.count(k)) return false;
				if (*v != *other.m_map.at(k)) return false;
			}
			return true;
		}
		bool operator!=(Object& other) const noexcept { return !((*this) == other); }
		bool operator==(const Element& other) const noexcept override
		{
			if (auto other_cast = dynamic_cast<const Object*>(&other)) return (*this) == (*other_cast);
			else return false;
		}
		bool operator!=(const Element& other) const noexcept override { return !((*this) == other); }
	};

	constexpr void Array::append(const int& val) { append_ptr(new Value(val)); }
	constexpr void Array::append(const float& val) { append_ptr(new Value(val)); }
	constexpr void Array::append(const bool& val) { append_ptr(new Value(val)); }
	constexpr void Array::append(const std::string& val) { append_ptr(new Value(val)); }
	constexpr void Array::append(const char* val) { append_ptr(new Value(val)); }
	constexpr void Array::append(const std::vector<Element*>& value) { append_ptr(new Array(value)); }
	constexpr void Array::append(const Map<std::string, Element*>& value) { append_ptr(new Object(value)); }
	
	inline Object::Object(const Array& other)
	{
		size_t idx = 0;
		for (auto& element : other.as_vector())
			insert_ptr(std::to_string(idx++), element->copy());
	}

	inline Object::Object(const Array* other)
	{
		size_t idx = 0;
		for (auto& element : other->as_vector())
			insert_ptr(std::to_string(idx++), element->copy());
	}

	inline void Object::insert(const std::string& property, const int& value) noexcept { insert_ptr(property, new Value(value)); }
	inline void Object::insert(const std::string& property, const float& value) noexcept { insert_ptr(property, new Value(value)); }
	inline void Object::insert(const std::string& property, const bool& value) noexcept { insert_ptr(property, new Value(value)); }
	inline void Object::insert(const std::string& property, const std::string& value) noexcept { insert_ptr(property, new Value(value)); }
	inline void Object::insert(const std::string& property, const std::vector<Element*>& value) noexcept { insert_ptr(property, new Array(value)); }
	inline void Object::insert(const std::string& property, const Map<std::string, Element*>& value) noexcept { insert_ptr(property, new Object(value)); }

	inline void Object::insert(const char* property, const int& value) noexcept { insert_ptr(property, new Value(value)); }
	inline void Object::insert(const char* property, const float& value) noexcept { insert_ptr(property, new Value(value)); }
	inline void Object::insert(const char* property, const bool& value) noexcept { insert_ptr(property, new Value(value)); }
	inline void Object::insert(const char* property, const std::string& value) noexcept { insert_ptr(property, new Value(value)); }
	inline void Object::insert(const char* property, const std::vector<Element*>& value) noexcept { insert_ptr(property, new Array(value)); }
	inline void Object::insert(const char* property, const Map<std::string, Element*>& value) noexcept { insert_ptr(property, new Object(value)); }

	// Parser Forehead Definition
	class InputStream;
	class Tokenizer;
	class Parser;

	// String Input Stream, faster? I guess...
	class InputStream
	{
	private:
		const std::string* str;
		const size_t begin, end;
		size_t current;
	public:
		InputStream(const std::string* str) : str(str), begin(0), current(0), end(str->size()) {}
		InputStream(const std::string* str, const size_t& begin, const size_t& end) : str(str), begin(begin), current(begin), end(end) {}
		InputStream(const InputStream& other) :str(other.str), begin(other.begin), current(other.current), end(other.end) {}
		InputStream(const InputStream& other, const size_t& begin, const size_t& end) :str(other.str), begin(begin), current(begin), end(end) {}
		~InputStream() = default;

	public:
		char get() { return (*str)[current++]; }
		char get(char& ch) { return ch = get(); }
		char peek() { return (*str)[current]; }
		char forward() { return (*str)[current + 1]; }
		bool eof() const noexcept { return current >= end; }
	};

	// Type of Token
	enum class TokenType {
		ObjectBegin, // '{'
		ObjectEnd, // '}'
		ArrayBegin, // '['
		ArrayEnd, // ']'
		Colon, // ':'
		Comma, // ','
		String, // "string"
		Integer, // 123
		Float, // 12.72
		Bool, // true or false
		Null, // null
		End // End of input
	};

	struct Token
	{
		TokenType type;
		std::string value;
	};

	// Tokenizer, turn string to tokens, and get them in a streamed way.
	class Tokenizer
	{
	private:
		InputStream stream;
		char current;
		size_t line;
		size_t column;

		Token current_token;
		// Get a fronted token, mainly using in parse_array()
		Token next_token;

		bool is_next_end = true;
	public:
		// Constructor
		Tokenizer(const InputStream& stream) :stream(stream), line(1), column(1) { next(); }
		Tokenizer(const Tokenizer& other) :
			stream(other.stream),
			current(other.current),
			line(other.line),
			column(other.column),
			current_token(other.current_token),
			next_token(other.next_token),
			is_next_end(other.is_next_end)
		{ }
		~Tokenizer() = default;

		// Token Getter
		const Token now() const noexcept { return current_token; }
		const Token forward() const noexcept { return next_token; }

		// Get the token (current_token = next_token, next_token updates, return current_token)
		inline const Token next()
		{
			skip_white_space();
			if (stream.eof())
			{
				if (is_next_end) return is_next_end = false, current_token = next_token;
				else return { TokenType::End, "" };
			}

			current = stream.get();
			column++;

			current_token = next_token;

			switch (current)
			{
			case '{':
				next_token = { TokenType::ObjectBegin, "{" };
				break;
			case '}':
				next_token = { TokenType::ObjectEnd, "}" };
				break;
			case '[':
				next_token = { TokenType::ArrayBegin, "[" };
				break;
			case ']':
				next_token = { TokenType::ArrayEnd, "]" };
				break;
			case ':':
				next_token = { TokenType::Colon, ":" };
				break;
			case ',':
				next_token = { TokenType::Comma, "," };
				break;
			case '"':
				next_token = parse_string();
				break;
			case 't':
			case 'f':
				next_token = parse_bool();
				break;
			case 'n':
				next_token = parse_null();
				break;
			default:
				if (isdigit(current) || current == '-') next_token = parse_number();
				else throw Exception("Unexpected character '" + std::string(1, current) + "' at line " + std::to_string(line) + ", column " + std::to_string(column));
				break;
			}

			return current_token;
		}

		// It's proved to be veeeeeeeeeery slow! Don't use it!
		std::vector<Token> get_all_tokens()
		{
			std::vector<Token> _arr;
			Token _current_token;
			while ((_current_token = next()).type != TokenType::End) _arr.push_back(_current_token);
			_arr.push_back(_current_token);
			return _arr;
		}

	private:
		// Skip white space. May have a faster way?
		inline void skip_white_space()
		{
			while (!stream.eof() && (stream.peek() == '\n' || stream.peek() == '\t' || stream.peek() == ' '))
			{
				current = stream.get();
				column++;
				if (current == '\n') line++, column = 1;
			}
		}

		// Parse Number Token
		Token parse_number()
		{
			std::string _str;
			bool is_float = false;

			_str.push_back(current);
			while (isdigit(stream.forward()) || stream.forward() == '.' || stream.forward() == '-')
			{
				stream.get(current);
				is_float = is_float || current == '.';
				_str.push_back(current);

				column++;
			}
			if (isdigit(stream.peek())) _str.push_back(stream.get());

			return { is_float ? TokenType::Float : TokenType::Integer, _str };
		}

		// Parse Boolean Token, not be much faster after optimization, so I keep it.
		Token parse_bool()
		{
			if (current == 't')
			{
				expect('r'), expect('u'), expect('e');
				return { TokenType::Bool, "true" };
			}
			else
			{
				expect('a'), expect('l'), expect('s'), expect('e');
				return { TokenType::Bool, "false" };
			}

		}

		Token parse_string()
		{
			std::string _str;
			while (stream.get(current))
			{
				column++;
				if (current == '"') break;
				if (current == '\\') stream.get(current);
				_str.push_back(current);
			}
			return { TokenType::String, _str };
		}

		Token parse_null()
		{
			expect('u'), expect('l'), expect('l');
			return { TokenType::Null, "null" };
		}

		void expect(char nextChar)
		{
			if ((current = stream.get()) != nextChar)
				throw Exception("Unexpected character '" + std::string(1, current) + "' at line " + std::to_string(line) + ", column " + std::to_string(column));
			column++;
		}
	};

	// Json parser, which uses tokenizer to parse json data.
	class Parser
	{
	public:
		// Functions
		static Element* parse(Tokenizer* tokenizer) { return Parser::parse_value(tokenizer); }

	private:
		static Element* parse_value(Tokenizer* tokenizer)
		{
			switch (tokenizer->next().type)
			{
			case TokenType::ObjectBegin:
				return Parser::parse_object(tokenizer);
			case TokenType::ArrayBegin:
				return Parser::parse_array(tokenizer);
			case TokenType::Integer:
				return new Value(std::atoi(tokenizer->now().value.c_str()));
			case TokenType::Float:
				try { return new Value((float)std::stod(tokenizer->now().value)); }
				catch (...) { throw Exception("Invalid float: " + tokenizer->now().value); }
			case TokenType::Bool:
				return new Value((!tokenizer->now().value.empty() && tokenizer->now().value[0] == 't'));
			case TokenType::String:
				return new Value(tokenizer->now().value);
			case TokenType::Null:
				return new Value();
			default:
				throw Exception("Unexpected token type");
			}
		}

		static Object* parse_object(Tokenizer* tokenizer)
		{
			Object* obj = new Object();
			Token tk = tokenizer->next();

			while (tk.type != TokenType::ObjectEnd)
			{
				if (tk.type != TokenType::String) throw Exception("Expected string key in object!");
				std::string key = tk.value;

				tk = tokenizer->next();
				if (tk.type != TokenType::Colon) throw Exception("Expected colon after key!");

				obj->insert_ptr(key, parse_value(tokenizer));

				tk = tokenizer->next();
				if (tk.type == TokenType::Comma) tk = tokenizer->next();
			}

			return obj;
		}

		static Array* parse_array(Tokenizer* tokenizer)
		{
			Array* arr = new Array();
			Token tk = tokenizer->forward();
			bool is_empty = true;

			while (tk.type != TokenType::ArrayEnd)
			{
				is_empty = false;
				arr->append_ptr(parse_value(tokenizer));
				tk = tokenizer->next();
			}
			if (is_empty) tk = tokenizer->next();

			return arr;
		}
	};

	class Utils
	{
	public:
		static std::stringstream readFileStream(const std::string& filename)
		{
			std::ifstream file(filename);
			std::stringstream buffer;
			if (!file.is_open())throw Exception("Unable to open file: " + filename);

			buffer << file.rdbuf();
			return std::move(buffer);
		}

		static std::string readFile(const std::string& filename) { return std::move(readFileStream(filename).str()); }

		static Element* parseJsonStream(std::string& file)
		{
			InputStream stream(&file, 0, file.size());
			Tokenizer tokenizer(stream);

			Element* element = Parser::parse(&tokenizer);
			return element;
		}

		static void functionWrapper(std::string func_name, std::function<void()> test_func)
		{
			std::cout << func_name + " started!\n";
			std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
			try
			{
				test_func();
				std::cout << func_name + " passed!\n";
			}
			catch (Exception e)
			{
				std::cout << "Exception: " << e.what() << '\n';
			}

			std::chrono::duration<double, std::milli> duration(std::chrono::steady_clock::now() - start);
			std::cout << "Spend " << duration.count() << " ms.\n";
		}
	};
}


#endif