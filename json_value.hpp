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
        /**
         * @class Value
         * @brief 表示一个 JSON 的基本值，如 null, bool, int, float, string。
         */
        class Value : public Element
        {
        private:
            value_t m_value;               // 使用 std::variant 存储具体的值
            static ObjectPool<Value> pool; // 用于 Value 对象的静态对象池

        public:
            /// @brief 默认构造函数，创建一个 null 值。
            Value() : m_value(nullptr) {}
            /// @brief 构造函数，创建指定类型的值。
            Value(std::nullptr_t) : m_value(nullptr) {}
            Value(bool p_value) : m_value(p_value) {}
            Value(int p_value) : m_value(p_value) {}
            Value(float p_value) : m_value(p_value) {}

            Value(const char *p_value) : m_value(string_t(p_value)) {}
            Value(const string_t &p_value) : m_value(p_value) {}
            Value(string_t &&p_value) : m_value(std::move(p_value)) {}
            Value(std::string_view p_value) : m_value(p_value) {}

            /// @brief 拷贝构造函数。
            Value(const Value &other) : m_value(other.m_value) {}
            Value(const Value *other) : m_value(other->m_value) {}
            /// @brief 移动构造函数。
            Value(Value &&other) noexcept : m_value(std::move(other.m_value)) { other.m_value = nullptr; }

            /// @brief 拷贝赋值运算符。
            Value &operator=(const Value &other)
            {
                if (this != &other)
                    this->m_value = other.m_value;
                return *this;
            }

            /// @brief 移动赋值运算符。
            Value &operator=(Value &&other) noexcept
            {
                if (this != &other)
                    this->m_value = std::move(other.m_value);
                return *this;
            }

            /// @brief 从 Element 基类进行赋值。
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
            /// @brief 覆写基类方法，表明这是一个 Value 类型。
            bool is_value() const noexcept override { return true; }
            /// @brief 覆写基类方法，返回 this 指针。
            Value *as_value() override { return this; }

        public:
            /// @brief 比较两个 Value 对象是否相等。
            bool operator==(Value &other) const noexcept
            {
                if (this == &other)
                    return true;
                return m_value == other.m_value;
            }
            bool operator!=(Value &other) const noexcept { return !((*this) == other); }

            /// @brief 比较 Value 和 Element 对象是否相等。
            bool operator==(const Element &other) const noexcept override
            {
                if (const Value *val = dynamic_cast<const Value *>(&other))
                    return (*this) == (*val);
                else
                    return false;
            }
            bool operator!=(const Element &other) const noexcept override { return !((*this) == other); }

        private:
            /// @brief 模板辅助函数，检查 m_value 是否持有特定类型 T。
            template <typename T>
            bool is_T() const noexcept { return std::holds_alternative<T>(m_value); }

        public:
            /// @brief 检查是否为 null。
            bool is_null() const noexcept { return is_T<std::nullptr_t>(); }
            /// @brief 检查是否为布尔值。
            bool is_bool() const noexcept { return is_T<bool>(); }
            /// @brief 检查是否为整数。
            bool is_int() const noexcept { return is_T<int>(); }
            /// @brief 检查是否为浮点数。
            bool is_float() const noexcept { return is_T<float>(); }
            /// @brief 检查是否为字符串。
            bool is_str() const noexcept { return is_T<string_t>() || is_T<string_v_t>(); }

        private:
            /// @brief 模板辅助函数，获取 m_value 中特定类型 T 的值。
            template <typename T>
            T as_T() const { return std::get<T>(m_value); }

        public:
            /// @brief 获取底层的 std::variant 值。
            value_t get_value() { return m_value; }

            /// @brief 获取布尔值，若类型不匹配则抛出异常。
            bool as_bool() const
            {
                if (is_bool())
                    return as_T<bool>();
                else
                    throw TypeException("Not bool type!");
            }

            /// @brief 获取整数值，允许从浮点数转换，否则抛出异常。
            int as_int() const
            {
                if (is_int())
                    return as_T<int>();
                else if (is_float())
                    return (int)as_T<float>();
                else
                    throw TypeException("Not int type!");
            }

            /// @brief 获取浮点数值，若类型不匹配则抛出异常。
            float as_float() const
            {
                if (is_float())
                    return as_T<float>();
                else
                    throw TypeException("Not float type!");
            }

            /// @brief 获取字符串值，若类型不匹配则抛出异常。
            string_t as_str() const
            {
                if (is_T<string_t>())
                    return as_T<string_t>();
                else if (is_T<string_v_t>())
                    return string_t(as_T<string_v_t>());
                else
                    throw TypeException("Not string type!");
            }

        public:
            /// @brief 创建并返回当前 Value 对象的深拷贝。
            Element *copy() const noexcept override { return new Value(this); }

            /// @brief 将 Value 序列化为紧凑字符串。
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

            /// @brief 将 Value 序列化为美化字符串（与紧凑版相同）。
            string_t pretty_serialize(size_t depth = 0, char = '\t') const noexcept override
            {
                return serialize();
            }

            /// @brief 重载 new 运算符，使用对象池进行内存分配。
            void *operator new(std::size_t n) { return Value::pool.allocate(n); }
            /// @brief 重载 delete 运算符，将内存归还给对象池。
            void operator delete(void *ptr) { Value::pool.deallocate(ptr); }
        };
        // 静态成员初始化
        inline ObjectPool<Value> Value::pool;
    }
}

#endif // INCLUDE_JSON_VALUE