#ifndef INCLUDE_JSON_VALUE
#define INCLUDE_JSON_VALUE

#include "json_definition.hpp"
#include "json_exception.hpp"
#include "json_element.hpp"
#include "object_pool.hpp"

namespace pjh_std
{
    namespace json
    {
        class Value : public Element
        {
        private:
            value_t m_value;
            static ObjectPool<Value> pool;

        public:
            Value() : m_value(nullptr) {}
            Value(std::nullptr_t) : m_value(nullptr) {}
            Value(bool p_value) : m_value(p_value) {}
            Value(int p_value) : m_value(p_value) {}
            Value(float p_value) : m_value(p_value) {}
            Value(const std::string &p_value) : m_value(p_value) {}
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

            Value &operator=(Element &&other) noexcept
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

            std::string as_str() const
            {
                if (is_str())
                    return as_T<std::string>();
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

            string_t pretty_serialize(size_t depth = 0, char = '\t') const noexcept override
            {
                return serialize();
            }

            void *operator new(size_t n) { Value::pool.allocate(n); }

            void operator delete(void *ptr) { Value::pool.deallocate(ptr); }
        };

        ObjectPool<Value> Value::pool;
    }
}

#endif // INCLUDE_JSON_VALUE