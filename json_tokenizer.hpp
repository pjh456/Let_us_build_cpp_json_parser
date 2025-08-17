#ifndef INCLUDE_JSON_TOKENIZER
#define INCLUDE_JSON_TOKENIZER

#include <string>

#include "json_definition.hpp"
#include "json_exception.hpp"

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
            char current_ch;
            size_t line;
            size_t column;

            Token m_current_token;

        public:
            // Constructor
            Tokenizer(const InputStream &stream) : stream(stream), line(1), column(1) { consume(); }
            Tokenizer(const Tokenizer &other) = default;
            ~Tokenizer() = default;

            // Token Getter
            const Token peek() const noexcept { return m_current_token; }

            void consume() { m_current_token = read_next_token(); }

        private:
            Token read_next_token()
            {
                skip_white_space();
                if (stream.eof())
                {
                    return {TokenType::End, ""};
                }

                current_ch = stream.get();
                column++;

                switch (current_ch)
                {
                case '{':
                    return {TokenType::ObjectBegin, "{"};
                case '}':
                    return {TokenType::ObjectEnd, "}"};
                case '[':
                    return {TokenType::ArrayBegin, "["};
                case ']':
                    return {TokenType::ArrayEnd, "]"};
                case ':':
                    return {TokenType::Colon, ":"};
                case ',':
                    return {TokenType::Comma, ","};
                case '"':
                    return parse_string();
                case 't':
                case 'f':
                    return parse_bool();
                case 'n':
                    return parse_null();
                default:
                    if (isdigit(current_ch) || current_ch == '-')
                        return parse_number();
                    else
                        throw ParseException(line, column, (std::string) "Unexpected character '" + std::string(1, current_ch) + "'");
                    break;
                }
            }

        private:
            // Skip white space. May have a faster way?
            inline void skip_white_space()
            {
                while (!stream.eof() && (stream.peek() == '\n' || stream.peek() == '\t' || stream.peek() == ' '))
                {
                    current_ch = stream.get();
                    column++;
                    if (current_ch == '\n')
                        line++, column = 1;
                }
            }

            // Parse Number Token
            Token parse_number()
            {
                std::string _str;
                bool is_float = false;

                _str.push_back(current_ch);
                while (isdigit(stream.forward()) || stream.forward() == '.' || stream.forward() == '-')
                {
                    stream.get(current_ch);
                    is_float = is_float || current_ch == '.';
                    _str.push_back(current_ch);

                    column++;
                }
                if (isdigit(stream.peek()))
                    _str.push_back(stream.get());

                return {is_float ? TokenType::Float : TokenType::Integer, _str};
            }

            // Parse Boolean Token, not be much faster after optimization, so I keep it.
            Token parse_bool()
            {
                if (current_ch == 't')
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
                while (stream.get(current_ch))
                {
                    column++;
                    if (current_ch == '"')
                        break;
                    if (current_ch == '\\')
                        stream.get(current_ch);
                    _str.push_back(current_ch);
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
                if ((current_ch = stream.get()) != nextChar)
                    throw ParseException(line, column, (std::string) "Unexpected character '" + std::string(1, current_ch) + "'");
                column++;
            }
        };
    }
}

#endif // INCLUDE_JSON_TOKENIZER