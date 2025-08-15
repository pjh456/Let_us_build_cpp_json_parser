#ifndef INCLUDE_JSON_PARSER
#define INCLUDE_JSON_PARSER

#include <string>
#include <vector>

#include "json_definition.hpp"
#include "json_exception.hpp"
#include "json_element.hpp"
#include "json_object.hpp"
#include "json_array.hpp"
#include "json_value.hpp"
#include "json_ref.hpp"

namespace pjh_std
{
    namespace json
    {
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
            inline const Token next()
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
            // Functions
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

#endif // INCLUDE_JSON_PARSER