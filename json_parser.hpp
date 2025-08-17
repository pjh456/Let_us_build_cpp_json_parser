#ifndef INCLUDE_JSON_PARSER
#define INCLUDE_JSON_PARSER

#include <thread>

#include "channel.hpp"
#include "object_pool.hpp"
#include "json_element.hpp"
#include "json_value.hpp"
#include "json_array.hpp"
#include "json_object.hpp"
#include "json_ref.hpp"
#include "json_tokenizer.hpp"
#include "lock_free_ring_buffer.hpp"

namespace pjh_std
{
    namespace json
    {
        class Parser
        {
        private:
            LockFreeRingBuffer<Token> m_buffer;
            Tokenizer *m_tokenizer;

        public:
            Parser(Tokenizer *p_tokenizer, size_t capacity = 256)
                : m_tokenizer(p_tokenizer), m_buffer(capacity) {}

            Ref parse()
            {
                std::thread producer_thread(
                    [&]()
                    {
                        try
                        {
                            while (true)
                            {
                                Token token = m_tokenizer->peek();
                                if (token.type == TokenType::End)
                                    break;
                                while (!m_buffer.push(token))
                                {
                                    std::this_thread::yield();
                                }
                                m_tokenizer->consume();
                            }
                        }
                        catch (const std::exception &e)
                        {
                            throw ThreadException(e.what());
                        }
                    });

                Ref root = Ref(Parser::parse_value());
                producer_thread.join();
                return root;
            }

        private:
            Token peek()
            {
                std::optional<Token> token;
                while (!(token = m_buffer.peek()).has_value())
                {
                    std::this_thread::yield();
                }
                return token.value();
            }
            void consume()
            {
                while (!m_buffer.pop())
                {
                    std::this_thread::yield();
                }
            }

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
                    int val = std::atoi(token.value.c_str());
                    consume();
                    return new Value(val);
                }
                case TokenType::Float:
                {
                    try
                    {
                        float val = (float)std::stod(token.value);
                        consume();
                        return new Value(val);
                    }
                    catch (...)
                    {
                        throw Exception("Invalid float: " + token.value);
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
                    string_t val = token.value;
                    consume();
                    return new Value(val);
                }
                case TokenType::Null:
                    consume();
                    return new Value();
                default:
                    throw TypeException("Unexpected token type");
                }
            }

            Object *parse_object()
            {
                consume();

                Object *obj = new Object();

                if (peek().type == TokenType::ObjectEnd)
                {
                    consume();
                    return obj;
                }

                while (true)
                {
                    Token key_token = peek();

                    if (key_token.type != TokenType::String)
                        throw Exception("Expected string key in object!");
                    std::string key = key_token.value;
                    consume();

                    if (peek().type != TokenType::Colon)
                        throw Exception("Expected colon after key!");
                    consume();

                    obj->insert_raw_ptr(key, parse_value());

                    Token next_token = peek();
                    if (next_token.type == TokenType::ObjectEnd)
                    {
                        consume();
                        break;
                    }
                    else if (next_token.type == TokenType::Comma)
                        consume();
                    else
                        throw Exception("Expected ',' or '}' in object");
                }

                return obj;
            }

            Array *parse_array()
            {
                consume();
                Array *arr = new Array();

                if (peek().type == TokenType::ArrayEnd)
                {
                    consume();
                    return arr;
                }

                while (true)
                {
                    arr->append_raw_ptr(parse_value());

                    Token next_token = peek();
                    if (next_token.type == TokenType::ArrayEnd)
                    {
                        consume();
                        break;
                    }
                    else if (next_token.type == TokenType::Comma)
                        consume();
                    else
                        throw Exception("Expected ',' or ']' in array");
                }

                return arr;
            }
        };
    }
}

#endif // INCLUDE_JSON_PARSER