#ifndef INCLUDE_JSON_EXCEPTION
#define INCLUDE_JSON_EXCEPTION

#include <stdexcept>

#include "json_definition.hpp"

namespace pjh_std
{
    namespace json
    {
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
    }
}

#endif // INCLUDE_JSON_EXCEPTION