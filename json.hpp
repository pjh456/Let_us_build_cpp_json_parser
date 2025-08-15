#ifndef INCLUDE_JSON
#define INCLUDE_JSON

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <algorithm>

namespace pjh_std
{
    namespace json
    {
        // Json values
        class Ref;
        class Element;

        class Value;
        class Object;
        class Array;

        using string_t = std::string;

        using value_t = std::variant<
            std::nullptr_t,
            bool,
            int,
            float,
            string_t>;

        template <typename T>
        using array_t = std::vector<T>;

        template <typename T>
        using object_t = std::map<string_t, T>;

        // Exceptions
        class Exception;

        class ParseException;
        class TypeException;
        class OutOfRangeException;
        class InvalidKeyException;
        class SerializationException;

        // Base Json Exception
        class Exception : public std::runtime_error
        {
        public:
            explicit Exception(const std::string &msg)
                : std::runtime_error(msg) {}
        };

        class ParseException : public Exception
        {
        public:
            ParseException(size_t p_line, size_t p_col, const std::string &msg)
                : Exception(
                      "Parse error at line " + std::to_string(p_line) +
                      ", column " + std::to_string(p_col) + ": " + msg),
                  m_line(p_line), m_col(p_col) {}

            size_t line() const noexcept { return m_line; }
            size_t column() const noexcept { return m_col; }

        private:
            size_t m_line, m_col;
        };

        class TypeException : public Exception
        {
        public:
            explicit TypeException(const std::string &msg)
                : Exception("Type error: " + msg) {}
        };

        class OutOfRangeException : public Exception
        {
        public:
            explicit OutOfRangeException(const std::string &msg)
                : Exception("Out of range: " + msg) {}
        };

        class InvalidKeyException : public Exception
        {
        public:
            explicit InvalidKeyException(const std::string &key)
                : Exception("Invalid key: '" + key + "'") {}
        };

        class SerializationException : public Exception
        {
        public:
            explicit SerializationException(const std::string &msg)
                : Exception("Serialization error: " + msg) {}
        };

        class NullPointerException : public Exception
        {
        public:
            explicit NullPointerException(const std::string &msg)
                : Exception("Null pointer error: " + msg) {}
        };

        class Element
        {
        public:
            virtual ~Element() {}

            virtual bool is_value() const noexcept { return false; }
            virtual bool is_array() const noexcept { return false; }
            virtual bool is_object() const noexcept { return false; }

            virtual Value *as_value() { throw TypeException("Invalid base type!"); }
            virtual Array *as_array() { throw TypeException("Invalid base type!"); }
            virtual Object *as_object() { throw TypeException("Invalid base type!"); }

            virtual void clear() {}
            virtual Element *copy() const = 0;
            virtual string_t serialize() const noexcept { return ""; }

            virtual bool operator==(const Element &other) const noexcept { return false; }
            virtual bool operator!=(const Element &other) const noexcept { return true; }
        };

        class Value : public Element
        {
        private:
            value_t m_value;

        public:
            Value() : m_value(nullptr) {}
            Value(bool p_value) : m_value(p_value) {}
            Value(int p_value) : m_value(p_value) {}
            Value(float p_value) : m_value(p_value) {}
            Value(const string_t &p_value) : m_value(p_value) {}
            Value(char *p_value) : Value((string_t)p_value) {}

            Value(const Value &other) : m_value(other.m_value) {}
            Value(const Value *other) : m_value(other->m_value) {}
            Value(Value &&other) noexcept : m_value(std::move(other.m_value)) { other.m_value = nullptr; }

            Value &operator=(const Value &other)
            {
                if (this != &other)
                    this->m_value = other.m_value;
                return *this;
            }

            Value &operator=(Value &&other) noexcept
            {
                if (this != &other)
                    this->m_value = std::move(other.m_value);
                return *this;
            }

            Value &operator=(const Element &other)
            {
                if (const Value *val = dynamic_cast<const Value *>(&other))
                    return operator=(*val);
                else
                    throw TypeException("assign failed!");
            }

            Value &operator=(Element &&other)
            {
                if (Value *val = dynamic_cast<Value *>(&other))
                    return operator=(std::move(*val));
                else
                    throw TypeException("assign failed!");
            }

            ~Value() override = default;

        public:
            bool is_value() const noexcept override { return true; }
            Value *as_value() override { return this; }

        public:
            bool operator==(Value &other) const noexcept
            {
                if (this == &other)
                    return true;
                return m_value == other.m_value;
            }

            bool operator!=(Value &other) const noexcept { return !((*this) == other); }

            bool operator==(const Element &other) const noexcept override
            {
                if (const Value *val = dynamic_cast<const Value *>(&other))
                    return (*this) == (*val);
                else
                    return false;
            }

            bool operator!=(const Element &other) const noexcept override { return !((*this) == other); }

        private:
            template <typename T>
            bool is_T() const noexcept { return std::holds_alternative<T>(m_value); }

        public:
            bool is_null() const noexcept { return is_T<std::nullptr_t>(); }
            bool is_bool() const noexcept { return is_T<bool>(); }
            bool is_int() const noexcept { return is_T<int>(); }
            bool is_float() const noexcept { return is_T<float>(); }
            bool is_str() const noexcept { return is_T<std::string>(); }

        private:
            template <typename T>
            T as_T() const { return std::get<T>(m_value); }

        public:
            value_t get_value() { return m_value; }

            bool as_bool() const
            {
                if (is_bool())
                    return as_T<bool>();
                else
                    throw TypeException("Not bool type!");
            }

            int as_int() const
            {
                if (is_int())
                    return as_T<int>();
                else if (is_float())
                    return (int)as_T<float>();
                else
                    throw TypeException("Not int type!");
            }

            float as_float() const
            {
                if (is_float())
                    return as_T<float>();
                else
                    throw TypeException("Not float type!");
            }

            string_t as_str() const
            {
                if (is_str())
                    return as_T<string_t>();
                else
                    throw TypeException("Not string type!");
            }

        public:
            Element *copy() const noexcept override { return new Value(this); }

            string_t serialize() const noexcept override
            {
                if (is_int())
                    return std::to_string(as_int());
                else if (is_float())
                    return std::to_string(as_float());
                else if (is_str())
                    return "\"" + as_str() + "\"";
                else if (is_bool())
                    return as_bool() ? "true" : "false";
                else if (is_null())
                    return "null";
                else
                    return "";
            }
        };

        class Array : public Element
        {
        private:
            array_t<Element *> m_arr;

        public:
            Array() : m_arr() {}
            Array(const size_t count) : m_arr(count) {}
            Array(const array_t<Element *> &val) noexcept { append_all_raw_ptr(val); }

            Array(const Array &other) { copy_and_append_all(other.as_vector()); }
            Array(const Array *other) { copy_and_append_all(other->as_vector()); }
            Array(Array &&other) noexcept : m_arr(std::move(other.m_arr)) {}

            Array &operator=(const Array &other)
            {
                if (this != &other)
                    this->m_arr = other.m_arr;
                return *this;
            }

            Array &operator=(Array &&other)
            {
                if (this != &other)
                    this->m_arr = std::move(other.m_arr);
                return *this;
            }

            Array &operator=(const Element &other)
            {
                if (const Array *arr = dynamic_cast<const Array *>(&other))
                    return operator=(*arr);
                else
                    throw TypeException("assign failed!");
            }

            Array &operator=(Element &&other)
            {
                if (Array *arr = dynamic_cast<Array *>(&other))
                    return operator=(std::move(*arr));
                else
                    throw TypeException("assign failed!");
            }

            ~Array() override { clear(); }

        public:
            bool is_array() const noexcept override { return true; }
            Array *as_array() override { return this; }

            Element *as_element() const noexcept { return size() == 1 ? m_arr[0] : nullptr; }
            array_t<Element *> as_vector() const noexcept { return m_arr; }

        public:
            void clear() override
            {
                for (const auto &it : m_arr)
                    if (it != nullptr)
                        it->clear(), delete it;
                m_arr.clear();
            }

            Element *copy() const noexcept override { return new Array(this); }

            string_t serialize() const noexcept override
            {
                string_t new_str;
                new_str.push_back('[');
                bool is_first = true;
                for (const auto &it : m_arr)
                {
                    if (is_first)
                        is_first = false;
                    else
                        new_str.push_back(',');
                    new_str += it->serialize();
                }
                new_str.push_back(']');
                return new_str;
            }

        public:
            bool operator==(const Array &other) const noexcept
            {
                if (this == &other)
                    return true;
                return m_arr.size() == other.m_arr.size() &&
                       std::equal(m_arr.begin(), m_arr.end(), other.m_arr.begin(),
                                  [](Element *a, Element *b)
                                  { return a && b && *a == *b; });
            }

            bool operator!=(const Array &other) const noexcept { return !((*this) == other); }

            bool operator==(const Element &other) const noexcept override
            {
                if (const Array *arr = dynamic_cast<const Array *>(&other))
                    return (*this) == (*arr);
                return false;
            }

            bool operator!=(const Element &other) const noexcept override { return !((*this) == other); }

        public:
            size_t size() const noexcept { return m_arr.size(); }
            bool empty() const noexcept { return m_arr.empty(); }
            bool contains(Element *p_child) { return std::find(m_arr.begin(), m_arr.end(), p_child) != m_arr.end(); }

        public:
            Element *operator[](size_t p_index) noexcept { return p_index < size() ? m_arr[p_index] : nullptr; }
            const Element *operator[](size_t p_index) const noexcept { return p_index < size() ? m_arr[p_index] : nullptr; }

            Element *at(size_t p_index)
            {
                auto ret = operator[](p_index);
                if (ret != nullptr)
                    return ret;
                else
                    throw OutOfRangeException("index is out of range!");
            }
            const Element *at(size_t p_index) const
            {
                auto ret = operator[](p_index);
                if (ret != nullptr)
                    return ret;
                else
                    throw OutOfRangeException("index is out of range!");
            }

        public:
            void set_raw_ptr(size_t idx, Element *child)
            {
                if (size() <= idx)
                    m_arr.resize(idx);
                if (m_arr[idx] != nullptr)
                    delete m_arr[idx];
                m_arr[idx] = child;
            }

            void append_raw_ptr(Element *child) { m_arr.push_back(child); }
            void append_all_raw_ptr(const std::vector<Element *> &children)
            {
                for (auto &child : children)
                    append_raw_ptr(child);
            }

            void copy_and_append(const Element &child) { append_raw_ptr(child.copy()); }
            void copy_and_append_all(const std::vector<Element *> &children)
            {
                for (auto &child : children)
                    copy_and_append(*child);
            }

            void append(bool p_value) { append_raw_ptr(new Value(p_value)); }
            void append(int p_value) { append_raw_ptr(new Value(p_value)); }
            void append(float p_value) { append_raw_ptr(new Value(p_value)); }
            void append(const string_t &p_value) { append_raw_ptr(new Value(p_value)); }
            void append(char *p_value) { append_raw_ptr(new Value(p_value)); }

        public:
            void erase(const size_t &idx) { m_arr.erase(m_arr.begin() + idx); }

            void remove(const Element *child)
            {
                if (child == nullptr)
                    return;
                auto it = std::find(m_arr.begin(), m_arr.end(), child);
                if (it != m_arr.end())
                    m_arr.erase(it);
            }
            void remove_all(const std::vector<Element *> &children)
            {
                for (const Element *child : children)
                    remove(child);
            }
        };
        class Object : public Element
        {
        private:
            object_t<Element *> m_obj;

        public:
            Object() { m_obj.clear(); };
            Object(const object_t<Element *> &val) noexcept { insert_all_raw_ptr(val); }

            Object(const Object &other) noexcept { copy_and_insert_all(other.m_obj); }
            Object(const Object *other) noexcept { copy_and_insert_all(other->m_obj); }
            Object(Object &&other) noexcept : m_obj(std::move(other.m_obj)) {}

            ~Object() override { clear(); }

        public:
            bool is_object() const noexcept override { return true; }
            Object *as_object() override { return this; }
            object_t<Element *> as_raw_ptr_map() const noexcept { return m_obj; }

        public:
            void clear() override
            {
                for (auto it = m_obj.begin(); it != m_obj.end(); ++it)
                    (it->second)->clear(), delete it->second;
                m_obj.clear();
            }

            Object *copy() const noexcept override { return new Object(this); }

            string_t serialize() const noexcept override
            {
                string_t new_str;
                new_str.push_back('{');
                bool is_first = true;
                for (const auto &[k, v] : m_obj)
                {
                    new_str.push_back('\"');
                    new_str += k;
                    new_str.push_back('\"');
                    new_str.push_back(':');
                    new_str += v->serialize();
                    if (is_first)
                        is_first = false;
                    else
                        new_str.push_back(',');
                }
                new_str.push_back('}');
                return new_str;
            }

        public:
            bool operator==(const Object &other) const noexcept
            {
                if (this == &other)
                    return true;
                return m_obj.size() == other.m_obj.size() &&
                       std::equal(m_obj.begin(), m_obj.end(), other.m_obj.begin(),
                                  [](
                                      const std::pair<string_t, Element *> &a,
                                      const std::pair<string_t, Element *> b)
                                  { return a.second && b.second && *(a.second) == *(b.second); });
            }

            bool operator!=(const Object &other) const noexcept { return !((*this) == other); }

            bool operator==(const Element &other) const noexcept override
            {
                if (const Object *arr = dynamic_cast<const Object *>(&other))
                    return (*this) == (*arr);
                return false;
            }

            bool operator!=(const Element &other) const noexcept override { return !((*this) == other); }

        public:
            size_t size() const noexcept { return m_obj.size(); }
            bool empty() const noexcept { return m_obj.empty(); }
            bool contains(const string_t &p_key) const noexcept { return m_obj.find(p_key) != m_obj.end(); }

        public:
            Element *operator[](const string_t &p_key)
            {
                auto it = m_obj.find(p_key);
                return (it != m_obj.end()) ? it->second : nullptr;
            }

            const Element *operator[](const string_t &p_key) const
            {
                auto it = m_obj.find(p_key);
                return (it != m_obj.end()) ? it->second : nullptr;
            }

            Element *get(const string_t &p_key)
            {
                auto ret = operator[](p_key);
                if (ret != nullptr)
                    return ret;
                else
                    throw InvalidKeyException("invalid key!");
            }

            const Element *get(const string_t &p_key) const
            {
                auto ret = operator[](p_key);
                if (ret != nullptr)
                    return ret;
                else
                    throw InvalidKeyException("invalid key!");
            }

        public:
            void insert_raw_ptr(const string_t &p_key, Element *child)
            {
                auto it = m_obj.find(p_key);
                if (it != m_obj.end())
                    delete it->second;
                m_obj[p_key] = child;
            }
            void insert_all_raw_ptr(const object_t<Element *> &other)
            {
                for (auto &child : other)
                    insert_raw_ptr(child.first, child.second);
            }

            void copy_and_insert(const string_t &property, const Element &child) { insert_raw_ptr(property, child.copy()); }
            void copy_and_insert_all(const object_t<Element *> &other) noexcept
            {
                for (const auto &child : other)
                    copy_and_insert(child.first, *(child.second));
            }

            void insert(const string_t &p_key, bool p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
            void insert(const string_t &p_key, int p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
            void insert(const string_t &p_key, float p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
            void insert(const string_t &p_key, const string_t &p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
            void insert(const string_t &p_key, char *p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
        };

        class Ref
        {
        private:
            Element *m_ptr;

        public:
            Ref(Element *p_ptr = nullptr) : m_ptr(p_ptr) {}

            Ref operator[](const string_t &p_key)
            {
                if (!m_ptr)
                    throw NullPointerException("Null reference");
                if (auto obj = dynamic_cast<Object *>(m_ptr))
                    return Ref(obj->get(p_key));
                throw TypeException("Not an object");
            }

            Ref operator[](size_t index)
            {
                if (!m_ptr)
                    throw NullPointerException("Null reference");
                if (auto arr = dynamic_cast<Array *>(m_ptr))
                    return Ref(arr->at(index));
                throw TypeException("Not an array");
            }

        public:
            size_t size()
            {
                if (m_ptr->is_array())
                    return m_ptr->as_array()->size();
                else if (m_ptr->is_object())
                    return m_ptr->as_object()->size();
                else
                    return 1;
            }

        public:
            bool is_null() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_null();
            }
            bool is_bool() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_bool();
            }

            bool is_int() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_int();
            }

            bool is_float() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_float();
            }

            bool is_str() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_str();
            }

        public:
            bool as_bool() const
            {
                if (is_bool())
                    return m_ptr->as_value()->as_bool();
                throw TypeException("Not an bool value");
            }

            int as_int() const
            {
                if (is_int())
                    return m_ptr->as_value()->as_int();
                throw TypeException("Not an int value");
            }

            float as_float() const
            {
                if (is_float())
                    return m_ptr->as_value()->as_float();
                throw TypeException("Not an float value");
            }

            string_t as_str() const
            {
                if (is_str())
                    return m_ptr->as_value()->as_str();
                throw TypeException("Not an string value");
            }

            Element *get() { return m_ptr; }
        };

        enum class TokenType
        {
            ObjectBegin, // '{'
            ObjectEnd,   // '}'
            ArrayBegin,  // '['
            ArrayEnd,    // ']'
            Colon,       // ':'
            Comma,       // ','
            String,      // "string"
            Integer,     // 123
            Float,       // 12.72
            Bool,        // true or false
            Null,        // null
            End          // End of input
        };

        struct Token
        {
            TokenType type;
            std::string value;
        };

        class InputStream
        {
        private:
            std::string str;
            const size_t begin, end;
            size_t current;

        public:
            InputStream(const std::string &str) : str(str), begin(0), current(0), end(str.size()) {}
            InputStream(const std::string &str, const size_t &begin, const size_t &end) : str(str), begin(begin), current(begin), end(end) {}
            InputStream(const InputStream &other) = default;
            InputStream(const InputStream &other, const size_t &begin, const size_t &end) : str(other.str), begin(begin), current(begin), end(end) {}
            ~InputStream() = default;

        public:
            char get() { return str[current++]; }
            char get(char &ch) { return ch = get(); }
            char peek() { return str[current]; }
            char forward() { return str[current + 1]; }
            bool eof() const noexcept { return current >= end; }
        };

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
            Tokenizer(const InputStream &stream) : stream(stream), line(1), column(1) { next(); }
            Tokenizer(const Tokenizer &other) = default;
            ~Tokenizer() = default;

            // Token Getter
            const Token now() const noexcept { return current_token; }
            const Token forward() const noexcept { return next_token; }

            // Get the token (current_token = next_token, next_token updates, return current_token)
            Token next()
            {
                skip_white_space();
                if (stream.eof())
                {
                    if (is_next_end)
                        return is_next_end = false, current_token = next_token;
                    else
                        return {TokenType::End, ""};
                }

                current = stream.get();
                column++;

                current_token = next_token;

                switch (current)
                {
                case '{':
                    next_token = {TokenType::ObjectBegin, "{"};
                    break;
                case '}':
                    next_token = {TokenType::ObjectEnd, "}"};
                    break;
                case '[':
                    next_token = {TokenType::ArrayBegin, "["};
                    break;
                case ']':
                    next_token = {TokenType::ArrayEnd, "]"};
                    break;
                case ':':
                    next_token = {TokenType::Colon, ":"};
                    break;
                case ',':
                    next_token = {TokenType::Comma, ","};
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
                    if (isdigit(current) || current == '-')
                        next_token = parse_number();
                    else
                        throw ParseException(line, column, (std::string) "Unexpected character '" + std::string(1, current) + "'");
                    break;
                }

                return current_token;
            }

            // It's proved to be veeeeeeeeeery slow! Don't use it!
            std::vector<Token> get_all_tokens()
            {
                std::vector<Token> _arr;
                Token _current_token;
                while ((_current_token = next()).type != TokenType::End)
                    _arr.push_back(_current_token);
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
                    if (current == '\n')
                        line++, column = 1;
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
                if (isdigit(stream.peek()))
                    _str.push_back(stream.get());

                return {is_float ? TokenType::Float : TokenType::Integer, _str};
            }

            // Parse Boolean Token, not be much faster after optimization, so I keep it.
            Token parse_bool()
            {
                if (current == 't')
                {
                    expect('r'), expect('u'), expect('e');
                    return {TokenType::Bool, "true"};
                }
                else
                {
                    expect('a'), expect('l'), expect('s'), expect('e');
                    return {TokenType::Bool, "false"};
                }
            }

            Token parse_string()
            {
                std::string _str;
                while (stream.get(current))
                {
                    column++;
                    if (current == '"')
                        break;
                    if (current == '\\')
                        stream.get(current);
                    _str.push_back(current);
                }
                return {TokenType::String, _str};
            }

            Token parse_null()
            {
                expect('u'), expect('l'), expect('l');
                return {TokenType::Null, "null"};
            }

            void expect(char nextChar)
            {
                if ((current = stream.get()) != nextChar)
                    throw ParseException(line, column, (std::string) "Unexpected character '" + std::string(1, current) + "'");
                column++;
            }
        };

        class Parser
        {
        public:
            static Ref parse(Tokenizer *tokenizer) { return Ref(Parser::parse_value(tokenizer)); }

        private:
            static Element *parse_value(Tokenizer *tokenizer)
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
                    try
                    {
                        return new Value((float)std::stod(tokenizer->now().value));
                    }
                    catch (...)
                    {
                        throw Exception("Invalid float: " + tokenizer->now().value);
                    }
                case TokenType::Bool:
                    return new Value((!tokenizer->now().value.empty() && tokenizer->now().value[0] == 't'));
                case TokenType::String:
                    return new Value(tokenizer->now().value);
                case TokenType::Null:
                    return new Value();
                default:
                    throw TypeException("Unexpected token type");
                }
            }

            static Object *parse_object(Tokenizer *tokenizer)
            {
                Object *obj = new Object();
                Token tk = tokenizer->next();

                while (tk.type != TokenType::ObjectEnd)
                {
                    if (tk.type != TokenType::String)
                        throw Exception("Expected string key in object!");
                    std::string key = tk.value;

                    tk = tokenizer->next();
                    if (tk.type != TokenType::Colon)
                        throw Exception("Expected colon after key!");

                    obj->insert_raw_ptr(key, parse_value(tokenizer));

                    tk = tokenizer->next();
                    if (tk.type == TokenType::Comma)
                        tk = tokenizer->next();
                }

                return obj;
            }

            static Array *parse_array(Tokenizer *tokenizer)
            {
                Array *arr = new Array();
                Token tk = tokenizer->forward();
                bool is_empty = true;

                while (tk.type != TokenType::ArrayEnd)
                {
                    is_empty = false;
                    arr->append_raw_ptr(parse_value(tokenizer));
                    tk = tokenizer->next();
                }
                if (is_empty)
                    tk = tokenizer->next();

                return arr;
            }
        };
    }
}

#endif // INCLUDE_JSON