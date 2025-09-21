#ifndef INCLUDE_JSON_REF
#define INCLUDE_JSON_REF

#include <initializer_list>
#include <ostream>

#include <pjh_json/helpers/json_definition.hpp>
#include <pjh_json/helpers/json_exception.hpp>
#include <pjh_json/datas/json_element.hpp>
#include <pjh_json/datas/json_object.hpp>
#include <pjh_json/datas/json_array.hpp>
#include <pjh_json/datas/json_value.hpp>

namespace pjh_std
{
    namespace json
    {
        /**
         * @class Ref
         * @brief 一个 Element 指针的包装类，提供了类似智能指针的功能和便捷的链式访问语法。
         *        例如，可以使用 `json_ref["key"][0]` 的方式来访问嵌套的 JSON 数据。
         * 它同时负责了指针的生命周期！
         */
        class Ref
        {
        private:
            Element *m_ptr; // 指向实际的 Element 对象

        public:
            /// @brief 构造函数，可以接受一个 Element 指针。
            Ref(Element *p_ptr = nullptr) : m_ptr(p_ptr) {}
            Ref(const Ref &other) : m_ptr(nullptr)
            {
                if (other.m_ptr->is_value())
                    m_ptr = new Value(other.m_ptr->as_value());
                else if (other.m_ptr->is_object())
                    m_ptr = new Object(other.m_ptr->as_object());
                else if (other.m_ptr->is_object())
                    m_ptr = new Array(other.m_ptr->as_array());
            }
            Ref &operator=(const Ref &other)
            {
                if (this == &other)
                    return *this;
                if (m_ptr != nullptr)
                    delete m_ptr;
                if (other.m_ptr->is_value())
                    m_ptr = new Value(other.m_ptr->as_value());
                else if (other.m_ptr->is_object())
                    m_ptr = new Object(other.m_ptr->as_object());
                else if (other.m_ptr->is_object())
                    m_ptr = new Array(other.m_ptr->as_array());
                else
                    m_ptr = nullptr;
                return *this;
            }
            Ref(Ref &&other) noexcept
                : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }
            Ref &operator=(Ref &&other) noexcept
            {
                if (this == &other)
                    return *this;
                m_ptr = other.m_ptr;
                other.m_ptr = nullptr;
                return *this;
            }

            ~Ref()
            {
                if (m_ptr != nullptr)
                    delete m_ptr;
            }

            /// @brief 重载 [] 运算符，用于访问 Object 的成员。
            Ref operator[](string_v_t p_key)
            {
                if (!m_ptr)
                    throw NullPointerException("Null reference");
                if (auto obj = m_ptr->as_object())
                    return Ref(obj->get(p_key));
                throw TypeException("Not an object");
            }
            const Ref operator[](string_v_t p_key) const
            {
                if (!m_ptr)
                    throw NullPointerException("Null reference");
                if (auto obj = m_ptr->as_object())
                    return Ref(obj->get(p_key));
                throw TypeException("Not an object");
            }

            /// @brief 重载 [] 运算符，用于访问 Array 的成员。
            Ref operator[](size_t index)
            {
                if (!m_ptr)
                    throw NullPointerException("Null reference");
                if (auto arr = dynamic_cast<Array *>(m_ptr))
                    return Ref(arr->at(index));
                throw TypeException("Not an array");
            }
            const Ref operator[](size_t index) const
            {
                if (!m_ptr)
                    throw NullPointerException("Null reference");
                if (auto arr = dynamic_cast<Array *>(m_ptr))
                    return Ref(arr->at(index));
                throw TypeException("Not an array");
            }

        public:
            /// @brief 获取所包装的 Array 或 Object 的大小。
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
            /// @brief 检查包装的元素是否为 null。
            bool is_null() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_null();
            }
            /// @brief 检查包装的元素是否为布尔值。
            bool is_bool() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_bool();
            }

            /// @brief 检查包装的元素是否为整数。
            bool is_int() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_int();
            }

            /// @brief 检查包装的元素是否为浮点数。
            bool is_float() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_float();
            }

            /// @brief 检查包装的元素是否为字符串。
            bool is_str() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_str();
            }

        public:
            /// @brief 以布尔值形式获取元素内容。
            bool as_bool() const
            {
                if (is_bool())
                    return m_ptr->as_value()->as_bool();
                throw TypeException("Not an bool value");
            }
            /// @brief 以整数形式获取元素内容。
            int as_int() const
            {
                if (is_int())
                    return m_ptr->as_value()->as_int();
                throw TypeException("Not an int value");
            }
            /// @brief 以浮点数形式获取元素内容。
            float as_float() const
            {
                if (is_float())
                    return m_ptr->as_value()->as_float();
                throw TypeException("Not an float value");
            }
            /// @brief 以字符串形式获取元素内容。
            string_t as_str() const
            {
                if (is_str())
                    return m_ptr->as_value()->as_str();
                throw TypeException("Not an string value");
            }

            /// @brief 获取底层的 Element 指针。
            Element *get() const { return m_ptr; }

        public:
            /// @brief 重载 << 运算符，以便将 Ref 对象直接输出到流（美化格式）。
            friend std::ostream &operator<<(std::ostream &os, Ref &ref)
            {
                os << ref.get()->pretty_serialize(0, ' ');
                return os;
            }
        };

        /**
         * @brief 工厂函数，使用初始化列表方便地创建一个 JSON Object。
         * @param p_list 键值对的初始化列表。
         * @return 包裹新创建的 Object 的 Ref 对象。
         */
        inline Ref make_object(std::initializer_list<std::pair<string_t, Ref>> p_list)
        {
            auto obj = new Object();
            for (auto &kv : p_list)
                obj->insert_raw_ptr(kv.first, kv.second.get());
            return Ref(obj);
        }

        /**
         * @brief 工厂函数，使用初始化列表方便地创建一个 JSON Array。
         * @param p_list 值的初始化列表。
         * @return 包裹新创建的 Array 的 Ref 对象。
         */
        inline Ref make_array(std::initializer_list<Ref> p_list)
        {
            auto arr = new Array();
            for (const auto &v : p_list)
                arr->append_raw_ptr(v.get());
            return Ref(arr);
        }

        /**
         * @brief 工厂函数，用于方便地创建各种类型的 JSON Value。
         * @return 包裹新创建的 Value 的 Ref 对象。
         */
        inline Ref make_value(std::nullptr_t) { return Ref(new Value(nullptr)); }
        inline Ref make_value(bool p_val) { return Ref(new Value(p_val)); }
        inline Ref make_value(int p_val) { return Ref(new Value(p_val)); }
        inline Ref make_value(float p_val) { return Ref(new Value(p_val)); }
        inline Ref make_value(const char *p_val) { return Ref(new Value((string_t)p_val)); }
        inline Ref make_value(const string_t p_val) { return Ref(new Value(p_val)); }
    }
}

#endif // INCLUDE JSON_REF