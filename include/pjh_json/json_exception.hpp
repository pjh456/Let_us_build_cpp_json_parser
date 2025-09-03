#ifndef INCLUDE_JSON_EXCEPTION
#define INCLUDE_JSON_EXCEPTION

#include <stdexcept>

#include "json_definition.hpp"

namespace pjh_std
{
    namespace json
    {
        /**
         * @class Exception
         * @brief 所有 JSON 库异常的基类，继承自 std::runtime_error。
         */
        class Exception : public std::runtime_error
        {
        public:
            /**
             * @brief 构造函数。
             * @param msg 异常信息。
             */
            explicit Exception(const std::string &msg)
                : std::runtime_error(msg) {}
        };

        /**
         * @class ParseException
         * @brief 解析 JSON 字符串时发生语法错误时抛出的异常。
         */
        class ParseException : public Exception
        {
        public:
            /**
             * @brief 构造函数，包含错误发生的行号和列号。
             * @param p_line 错误发生的行号。
             * @param p_col 错误发生的列号。
             * @param msg 具体的错误信息。
             */
            ParseException(size_t p_line, size_t p_col, const std::string &msg)
                : Exception(
                      "Parse error at line " + std::to_string(p_line) +
                      ", column " + std::to_string(p_col) + ": " + msg),
                  m_line(p_line), m_col(p_col) {}

            /// @brief 获取错误行号。
            size_t line() const noexcept { return m_line; }
            /// @brief 获取错误列号。
            size_t column() const noexcept { return m_col; }

        private:
            size_t m_line, m_col; // 存储错误位置的行号和列号
        };

        /**
         * @class TypeException
         * @brief 当尝试以错误的类型访问 JSON 值时抛出的异常。
         */
        class TypeException : public Exception
        {
        public:
            explicit TypeException(const std::string &msg)
                : Exception("Type error: " + msg) {}
        };

        /**
         * @class OutOfRangeException
         * @brief 当访问 JSON 数组的索引超出范围时抛出的异常。
         */
        class OutOfRangeException : public Exception
        {
        public:
            explicit OutOfRangeException(const std::string &msg)
                : Exception("Out of range: " + msg) {}
        };

        /**
         * @class InvalidKeyException
         * @brief 当使用不存在的键访问 JSON 对象时抛出的异常。
         */
        class InvalidKeyException : public Exception
        {
        public:
            explicit InvalidKeyException(const std::string &key)
                : Exception("Invalid key: '" + key + "'") {}
        };

        /**
         * @class SerializationException
         * @brief 在序列化 JSON 对象为字符串时发生错误时抛出的异常。
         */
        class SerializationException : public Exception
        {
        public:
            explicit SerializationException(const std::string &msg)
                : Exception("Serialization error: " + msg) {}
        };

        /**
         * @class NullPointerException
         * @brief 当试图通过空引用或空指针访问 JSON 元素时抛出的异常。
         */
        class NullPointerException : public Exception
        {
        public:
            explicit NullPointerException(const std::string &msg)
                : Exception("Null pointer error: " + msg) {}
        };

        /**
         * @class ThreadException
         * @brief 在多线程操作中发生错误时抛出的异常。
         */
        class ThreadException : public Exception
        {
        public:
            explicit ThreadException(const std::string &msg)
                : Exception("Thread error: " + msg) {}
        };
    }
}

#endif // INCLUDE_JSON_EXCEPTION