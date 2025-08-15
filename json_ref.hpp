#ifndef INCLUDE JSON_REF
#define INCLUDE JSON_REF

#include "json_definition.hpp"
#include "json_exception.hpp"
#include "json_element.hpp"
#include "json_object.hpp"
#include "json_array.hpp"
#include "json_value.hpp"

namespace pjh_std
{
    namespace json
    {
        class Ref
        {
        private:
            Element *m_ptr;

        public:
            Ref(Element *p_ptr = nullptr) : m_ptr(p_ptr) {}

            Ref operator[](const std::string &p_key)
            {
                if (!m_ptr)
                    throw NullPointerException("Null reference");
                if (auto obj = dynamic_cast<Object *>(m_ptr))
                    return Ref(obj->get(p_key));
                throw TypeException("Not an object");
            }

            Ref operator[](size_t index)
            {
                if (!m_ptr)
                    throw NullPointerException("Null reference");
                if (auto arr = dynamic_cast<Array *>(m_ptr))
                    return Ref(arr->at(index));
                throw TypeException("Not an array");
            }

        public:
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
            bool is_null() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_null();
            }
            bool is_bool() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_bool();
            }

            bool is_int() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_int();
            }

            bool is_float() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_float();
            }

            bool is_str() const
            {
                if (!m_ptr->is_value())
                    return false;
                return m_ptr->as_value()->is_str();
            }

        public:
            bool as_bool() const
            {
                if (is_bool())
                    return m_ptr->as_value()->as_bool();
                throw TypeException("Not an bool value");
            }

            int as_int() const
            {
                if (is_int())
                    return m_ptr->as_value()->as_int();
                throw TypeException("Not an int value");
            }

            float as_float() const
            {
                if (is_float())
                    return m_ptr->as_value()->as_float();
                throw TypeException("Not an float value");
            }

            string_t as_str() const
            {
                if (is_str())
                    return m_ptr->as_value()->as_str();
                throw TypeException("Not an string value");
            }

            Element *get() { return m_ptr; }
        };
    }
}

#endif // INCLUDE JSON_REF