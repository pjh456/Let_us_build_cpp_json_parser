#ifndef INCLUDE_JSON_ARRAY
#define INCLUDE_JSON_ARRAY

#include <algorithm>

#include "json_definition.hpp"
#include "json_exception.hpp"
#include "json_element.hpp"
#include "json_value.hpp"

namespace pjh_std
{
    namespace json
    {
        class Array : public Element
        {
        private:
            array_t<Element *> m_arr;

        public:
            Array() : m_arr() {}
            Array(const size_t count) : m_arr(count) {}
            Array(const array_t<Element *> &val) noexcept { append_all_raw_ptr(val); }

            Array(const Array &other) { copy_and_append_all(other.as_vector()); }
            Array(const Array *other) { copy_and_append_all(other->as_vector()); }
            Array(Array &&other) noexcept : m_arr(std::move(other.m_arr)) {}

            Array &operator=(const Array &other)
            {
                if (this != &other)
                    this->m_arr = other.m_arr;
                return *this;
            }

            Array &operator=(Array &&other)
            {
                if (this != &other)
                    this->m_arr = std::move(other.m_arr);
                return *this;
            }

            Array &operator=(const Element &other)
            {
                if (const Array *arr = dynamic_cast<const Array *>(&other))
                    return operator=(*arr);
                else
                    throw TypeException("assign failed!");
            }

            Array &operator=(Element &&other)
            {
                if (Array *arr = dynamic_cast<Array *>(&other))
                    return operator=(std::move(*arr));
                else
                    throw TypeException("assign failed!");
            }

            ~Array() override { clear(); }

        public:
            bool is_array() const noexcept override { return true; }
            Array *as_array() override { return this; }

            Element *as_element() const noexcept { return size() == 1 ? m_arr[0] : nullptr; }
            array_t<Element *> as_vector() const noexcept { return m_arr; }

        public:
            void clear() override
            {
                for (const auto &it : m_arr)
                    if (it != nullptr)
                        it->clear(), delete it;
                m_arr.clear();
            }

            Element *copy() const noexcept override { return new Array(this); }

            std::string serialize() const noexcept override
            {
                std::string new_str;
                new_str.push_back('[');
                bool is_first = true;
                for (const auto &it : m_arr)
                {
                    if (is_first)
                        is_first = false;
                    else
                        new_str.push_back(',');
                    new_str += it->serialize();
                }
                new_str.push_back(']');
                return new_str;
            }

        public:
            bool operator==(const Array &other) const noexcept
            {
                if (this == &other)
                    return true;
                return m_arr.size() == other.m_arr.size() &&
                       std::equal(m_arr.begin(), m_arr.end(), other.m_arr.begin(),
                                  [](Element *a, Element *b)
                                  { return a && b && *a == *b; });
            }

            bool operator!=(const Array &other) const noexcept { return !((*this) == other); }

            bool operator==(const Element &other) const noexcept override
            {
                if (const Array *arr = dynamic_cast<const Array *>(&other))
                    return (*this) == (*arr);
                return false;
            }

            bool operator!=(const Element &other) const noexcept override { return !((*this) == other); }

        public:
            size_t size() const noexcept { return m_arr.size(); }
            bool empty() const noexcept { return m_arr.empty(); }
            bool contains(Element *p_child) { return std::find(m_arr.begin(), m_arr.end(), p_child) != m_arr.end(); }

        public:
            Element *operator[](size_t p_index) noexcept { return p_index < size() ? m_arr[p_index] : nullptr; }
            const Element *operator[](size_t p_index) const noexcept { return p_index < size() ? m_arr[p_index] : nullptr; }

            Element *at(size_t p_index)
            {
                auto ret = operator[](p_index);
                if (ret != nullptr)
                    return ret;
                else
                    throw OutOfRangeException("index is out of range!");
            }
            const Element *at(size_t p_index) const
            {
                auto ret = operator[](p_index);
                if (ret != nullptr)
                    return ret;
                else
                    throw OutOfRangeException("index is out of range!");
            }

        public:
            void set_raw_ptr(size_t idx, Element *child)
            {
                if (size() <= idx)
                    m_arr.resize(idx);
                if (m_arr[idx] != nullptr)
                    delete m_arr[idx];
                m_arr[idx] = child;
            }

            void append_raw_ptr(Element *child) { m_arr.push_back(child); }
            void append_all_raw_ptr(const std::vector<Element *> &children)
            {
                for (auto &child : children)
                    append_raw_ptr(child);
            }

            void copy_and_append(const Element &child) { append_raw_ptr(child.copy()); }
            void copy_and_append_all(const std::vector<Element *> &children)
            {
                for (auto &child : children)
                    copy_and_append(*child);
            }

            void append(bool p_value) { append_raw_ptr(new Value(p_value)); }
            void append(int p_value) { append_raw_ptr(new Value(p_value)); }
            void append(float p_value) { append_raw_ptr(new Value(p_value)); }
            void append(const string_t &p_value) { append_raw_ptr(new Value(p_value)); }
            void append(char *p_value) { append_raw_ptr(new Value(p_value)); }

        public:
            void erase(const size_t &idx) { m_arr.erase(m_arr.begin() + idx); }

            void remove(const Element *child)
            {
                if (child == nullptr)
                    return;
                auto it = std::find(m_arr.begin(), m_arr.end(), child);
                if (it != m_arr.end())
                    m_arr.erase(it);
            }
            void remove_all(const std::vector<Element *> &children)
            {
                for (const Element *child : children)
                    remove(child);
            }
        };
    }
}

#endif // INCLUDE_JSON_ARRAY