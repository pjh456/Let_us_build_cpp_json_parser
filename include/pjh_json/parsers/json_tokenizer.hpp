#ifndef INCLUDE_JSON_TOKENIZER
#define INCLUDE_JSON_TOKENIZER

#include <string>

#include <pjh_json/helpers/json_definition.hpp>
#include <pjh_json/helpers/json_exception.hpp>

namespace pjh_std
{
    namespace json
    {

        // 定义 JSON 解析过程中的各种词法单元（Token）类型
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

        // 定义 Token 结构体，用于存储词法单元的类型和对应的值
        struct Token
        {
            TokenType type;
            std::string_view value;
        };

        /**
         * @class Tokenizer
         * @brief 词法分析器，负责将输入的 JSON 字符串分解成一系列的 Token。
         */
        class Tokenizer
        {
        private:
            std::string m_str; // 要解析的原始字符串
            size_t m_pos;      // 当前解析位置

            size_t line;   // 当前行号
            size_t column; // 当前列号

            Token m_current_token; // 当前已解析出的 Token

        public:
            /// @brief 构造函数。@param p_str 要解析的 JSON 字符串。
            Tokenizer(const std::string &p_str)
                : m_str(p_str), m_pos(0),
                  line(1), column(1) { consume(); } // 初始化时即读取第一个 token
            Tokenizer(const Tokenizer &other) = default;
            ~Tokenizer() = default;

            /// @brief 查看当前的 Token，但不移动解析位置。
            const Token peek() const noexcept { return m_current_token; }
            /// @brief 消费当前的 Token，并读取下一个 Token。
            void consume() { m_current_token = read_next_token(); }

        private:
            /// @brief 检查是否已到达字符串末尾。
            bool eof() const noexcept { return m_pos >= m_str.size(); }
            /// @brief 查看当前位置的字符，但不移动位置。
            char peek_char() const { return m_str[m_pos]; }
            /// @brief 获取当前位置的字符，并移动位置。
            char get_char() { return column++, m_str[m_pos++]; }

            /// @brief 读取并返回下一个有效的 Token。
            Token read_next_token()
            {
                // 1. 跳过所有空白字符
                skip_white_space();
                if (eof())
                    return {TokenType::End, ""}; // 如果已到末尾，返回 End Token

                const size_t start_pos = m_pos;
                char current_ch = peek_char();

                // 2. 根据当前字符判断 Token 类型
                switch (current_ch)
                {
                case '{':
                    get_char();
                    return {TokenType::ObjectBegin, std::string_view(m_str.data() + start_pos, 1)};
                case '}':
                    get_char();
                    return {TokenType::ObjectEnd, std::string_view(m_str.data() + start_pos, 1)};
                case '[':
                    get_char();
                    return {TokenType::ArrayBegin, std::string_view(m_str.data() + start_pos, 1)};
                case ']':
                    get_char();
                    return {TokenType::ArrayEnd, std::string_view(m_str.data() + start_pos, 1)};
                case ':':
                    get_char();
                    return {TokenType::Colon, std::string_view(m_str.data() + start_pos, 1)};
                case ',':
                    get_char();
                    return {TokenType::Comma, std::string_view(m_str.data() + start_pos, 1)};
                case '"':
                    return parse_string(); // 解析字符串
                case 't':
                case 'f':
                    return parse_bool(); // 解析布尔值
                case 'n':
                    return parse_null(); // 解析 null
                default:
                    if (isdigit(current_ch) || current_ch == '-')
                        return parse_number(); // 解析数字
                    else
                        throw ParseException(line, column, (std::string) "Unexpected character '" + std::string(1, current_ch) + "'");
                    break;
                }
            }

        private:
            /// @brief 跳过空白字符（空格、制表符、换行符）。
            inline void skip_white_space()
            {
                while (!eof() &&
                       (peek_char() == '\n' ||
                        peek_char() == '\t' ||
                        peek_char() == '\r' ||
                        peek_char() == ' '))
                {
                    if (get_char() == '\n') // 如果是换行符，更新行号和列号
                        line++, column = 1;
                }
            }

            /// @brief 解析数字 Token (整数或浮点数)。
            Token parse_number()
            {
                const size_t start_pos = m_pos;
                bool is_float = false;

                // 1. 处理可选的负号
                if (!eof() && peek_char() == '-')
                    get_char();

                // 2. 读取整数部分
                while (!eof() && isdigit(peek_char()))
                    get_char();

                // 3. 如果有小数点，则标记为浮点数并读取小数部分
                if (!eof() && peek_char() == '.')
                {
                    is_float = true;
                    get_char();

                    while (!eof() && isdigit(peek_char()))
                        get_char();
                }

                return {
                    is_float ? TokenType::Float : TokenType::Integer,
                    std::string_view(m_str.data() + start_pos, m_pos - start_pos)};
            }

            /// @brief 解析布尔值 Token (true 或 false)。
            Token parse_bool()
            {
                using namespace std::literals::string_view_literals;

                const size_t start_pos = m_pos;
                // 1. 尝试匹配 "true"
                if (m_str.size() - start_pos >= 4 && std::string_view(m_str.data() + start_pos, 4) == "true"sv)
                {
                    m_pos += 4;
                    column += 4;
                    return {TokenType::Bool, std::string_view(m_str.data() + start_pos, 4)};
                }
                // 2. 尝试匹配 "false"
                if (m_str.size() - start_pos >= 5 && std::string_view(m_str.data() + start_pos, 5) == "false"sv)
                {
                    m_pos += 5;
                    column += 5;
                    return {TokenType::Bool, std::string_view(m_str.data() + start_pos, 5)};
                }
                // 3. 匹配失败，抛出异常
                throw ParseException(line, column, "Invalid boolean literal");
            }

            /// @brief 解析字符串 Token。
            Token parse_string()
            {
                // 1. 消费开头的引号 "
                get_char();
                const size_t start_pos_content = m_pos;
                while (!eof())
                {
                    char ch = peek_char();
                    // 2. 处理转义字符
                    if (ch == '\\')
                    {
                        get_char(); // 消费 '\'
                        if (!eof())
                            get_char(); // 消费被转义的字符
                    }
                    // 3. 遇到结尾的引号，字符串结束
                    else if (ch == '"')
                    {
                        size_t end_pos_content = m_pos;
                        get_char(); // 消费结尾的引号 "
                        return {TokenType::String, std::string_view(m_str.data() + start_pos_content, end_pos_content - start_pos_content)};
                    }
                    // 4. 处理普通字符
                    else
                        get_char();
                }
                // 5. 如果直到字符串末尾也没找到闭合引号，则抛出异常
                throw ParseException(line, column, "Unterminated string literal");
            }

            /// @brief 解析 null Token。
            Token parse_null()
            {
                using namespace std::literals::string_view_literals;

                const size_t start_pos = m_pos;
                // 1. 尝试匹配 "null"
                if (m_str.size() - start_pos >= 4 && std::string_view(m_str.data() + start_pos, 4) == "null"sv)
                {
                    m_pos += 4;
                    column += 4;
                    return {TokenType::Null, std::string_view(m_str.data() + start_pos, 4)};
                }
                // 2. 匹配失败，抛出异常
                throw ParseException(line, column, "Invalid null literal");
            }
        };

    }
}

#endif // INCLUDE_JSON_TOKENIZER