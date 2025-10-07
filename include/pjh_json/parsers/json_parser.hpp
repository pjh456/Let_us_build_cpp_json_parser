#ifndef INCLUDE_JSON_PARSER
#define INCLUDE_JSON_PARSER

// #include <thread>
#include <charconv>

#include <pjh_json/datas/json_element.hpp>
#include <pjh_json/datas/json_value.hpp>
#include <pjh_json/datas/json_array.hpp>
#include <pjh_json/datas/json_object.hpp>

#include <pjh_json/helpers/json_ref.hpp>
#include <pjh_json/helpers/json_ref.hpp>

#include <pjh_json/parsers/json_tokenizer.hpp>

#include <pjh_json/utils/channel.hpp>
#include <pjh_json/utils/object_pool.hpp>
#include <pjh_json/utils/lock_free_ring_buffer.hpp>

namespace pjh_std
{
    namespace json
    {
        /**
         * @class Parser
         * @brief 语法分析器，采用递归下降的方式将 Token 流解析成一棵由 Element 构成的 JSON 树。
         *
         * 注意！如果需要跨 Parser 生命周期使用解析的数据，必须避免 Parser 的析构！
         * Parser 中保留有数据里 string_view 的原始指针！
         */
        class Parser
        {
        private:
            // LockFreeRingBuffer<Token> m_buffer;
            Tokenizer m_tokenizer; // 内嵌一个词法分析器

        public:
            /// @brief 构造函数。@param p_str 要解析的 JSON 字符串。
            Parser(const std::string &p_str /*, size_t capacity = 16384*/)
                : m_tokenizer(p_str) /*, m_buffer(capacity)*/ {}
            /// @brief 构造函数。@param p_tokenizer 外部传入的词法分析器。
            Parser(Tokenizer &p_tokenizer /*, size_t capacity*/)
                : m_tokenizer(p_tokenizer) /*, m_buffer(capacity)*/ {}

            /// @brief 解析的入口函数，开始整个解析过程。
            Ref parse() { return Ref(parse_value()); }

            // 多线程版本，但是性能不如单线程改回去了
            // Ref parse()
            // {
            //     std::thread producer_thread(
            //         [&]()
            //         {
            //             try
            //             {
            //                 while (true)
            //                 {
            //                     Token token = m_tokenizer.peek();
            //                     if (token.type == TokenType::End)
            //                         break;
            //                     while (!m_buffer.push(token))
            //                     {
            //                         std::this_thread::sleep_for(std::chrono::microseconds(10));
            //                         // std::cout << "full!" << std::endl;
            //                     }
            //                     m_tokenizer.consume();
            //                 }
            //             }
            //             catch (const std::exception &e)
            //             {
            //                 // std::cout << e.what() << std::endl;
            //                 throw ThreadException(e.what());
            //             }
            //         });

            //     Ref root = Ref(parse_value());
            //     producer_thread.join();
            //     return root;
            // }

        private:
            /// @brief 查看下一个 Token。
            Token peek() { return m_tokenizer.peek(); }
            /// @brief 消费当前 Token。
            void consume() { m_tokenizer.consume(); }
            // 多线程版本的跨线程交互数据
            // Token peek()
            // {
            //     std::optional<Token> token;
            //     while (!(token = m_buffer.peek()).has_value())
            //     {
            //         std::this_thread::sleep_for(std::chrono::microseconds(10));
            //         // std::cout << "empty!" << std::endl;
            //     }
            //     return token.value();
            // }
            // void consume()
            // {
            //     while (!m_buffer.pop())
            //     {
            //         std::this_thread::sleep_for(std::chrono::microseconds(10));
            //         // std::cout << "empty!" << std::endl;
            //     }
            // }

            /// @brief 解析一个通用的 JSON 值（可能是 object, array, string, number, bool, null）。
            /// 这是递归下降的核心分发函数。
            Element *parse_value()
            {
                Token token = peek();

                switch (token.type)
                {
                case TokenType::ObjectBegin:
                    return Parser::parse_object();
                case TokenType::ArrayBegin:
                    return Parser::parse_array();
                case TokenType::Integer:
                {
                    int val;
                    // 使用 C++17 的 from_chars 高效转换
                    auto [ptr, ec] = std::from_chars(token.value.data(), token.value.data() + token.value.size(), val);
                    if (ec != std::errc())
                    {
                        throw Exception("Invalid integer: " + std::string(token.value));
                    }
                    consume();
                    return new Value(val);
                }
                case TokenType::Float:
                {
                    try
                    {
                        // stof 需要一个有所有权的 string，所以创建一个临时对象
                        std::string temp_str(token.value);
                        float val = std::stof(temp_str);
                        consume();
                        return new Value(val);
                    }
                    catch (...)
                    {
                        throw Exception("Invalid float: " + std::string(token.value));
                    }
                }
                case TokenType::Bool:
                {
                    bool val = token.value[0] == 't';
                    consume();
                    return new Value(val);
                }
                case TokenType::String:
                {
                    auto val = token.value;
                    // string_t copied_str(token.value);
                    consume();
                    return new Value(val);
                    // return new Value(std::move(copied_str));
                }
                case TokenType::Null:
                    consume();
                    return new Value();
                default:
                    throw TypeException("Unexpected token type");
                }
            }

            /// @brief 解析一个 JSON 对象。
            Object *parse_object()
            {
                // 1. 消费 '{'
                consume();

                Object *obj = new Object();

                // 2. 处理空对象 {} 的情况
                if (peek().type == TokenType::ObjectEnd)
                {
                    consume();
                    return obj;
                }

                // 3. 循环解析键值对
                while (true)
                {
                    // 3.1 解析键（必须是字符串）
                    Token key_token = peek();
                    if (key_token.type != TokenType::String)
                        throw Exception("Expected string key in object!");
                    auto key = key_token.value;
                    consume();

                    // 3.2 消费 ':'
                    if (peek().type != TokenType::Colon)
                        throw Exception("Expected colon after key!");
                    consume();

                    // 3.3 递归解析值，并插入到对象中
                    obj->insert_raw_ptr(key, parse_value());

                    // 3.4 查看下一个 token 是 '}' 还是 ','
                    Token next_token = peek();
                    if (next_token.type == TokenType::ObjectEnd)
                    {
                        consume(); // 消费 '}'
                        break;     // 对象解析结束
                    }
                    else if (next_token.type == TokenType::Comma)
                        consume(); // 消费 ','，继续循环
                    else
                        throw Exception("Expected ',' or '}' in object");
                }

                return obj;
            }

            /// @brief 解析一个 JSON 数组。
            Array *parse_array()
            {
                // 1. 消费 '['
                consume();
                Array *arr = new Array();

                // 2. 处理空数组 [] 的情况
                if (peek().type == TokenType::ArrayEnd)
                {
                    consume();
                    return arr;
                }

                // 3. 循环解析数组成员
                while (true)
                {
                    // 3.1 递归解析数组成员的值
                    arr->append_raw_ptr(parse_value());

                    // 3.2 查看下一个 token 是 ']' 还是 ','
                    Token next_token = peek();
                    if (next_token.type == TokenType::ArrayEnd)
                    {
                        consume(); // 消费 ']'
                        break;     // 数组解析结束
                    }
                    else if (next_token.type == TokenType::Comma)
                        consume(); // 消费 ','，继续循环
                    else
                        throw Exception("Expected ',' or ']' in array");
                }

                return arr;
            }
        };
    }
}

#endif // INCLUDE_JSON_PARSER