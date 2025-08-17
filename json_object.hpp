#ifndef INCLUDE_JSON_OBJECT
#define INCLUDE_JSON_OBJECT

#include <algorithm>

#include "json_definition.hpp"
#include "json_exception.hpp"
#include "json_element.hpp"
#include "json_value.hpp"
#include "object_pool.hpp"

namespace pjh_std
{
    namespace json
    {
        class Object : public Element
        {
        private:
            object_t<Element *> m_obj;
            static ObjectPool<Object> pool;

        public:
            Object() { m_obj.clear(); };
            Object(const object_t<Element *> &val) noexcept { insert_all_raw_ptr(val); }

            Object(const Object &other) noexcept { copy_and_insert_all(other.m_obj); }
            Object(const Object *other) noexcept { copy_and_insert_all(other->m_obj); }
            Object(Object &&other) noexcept : m_obj(std::move(other.m_obj)) {}

            ~Object() override { clear(); }

        public:
            bool is_object() const noexcept override { return true; }
            Object *as_object() override { return this; }
            object_t<Element *> as_raw_ptr_map() const noexcept { return m_obj; }

        public:
            void clear() override
            {
                for (auto it = m_obj.begin(); it != m_obj.end(); ++it)
                    (it->second)->clear(), delete it->second;
                m_obj.clear();
            }

            Object *copy() const noexcept override { return new Object(this); }

            string_t serialize() const noexcept override
            {
                std::ostringstream oss;
                oss << '{';
                bool is_first = true;
                for (const auto &[k, v] : m_obj)
                {
                    oss << '\"' << k << '\"' << ':' << v->serialize();
                    if (is_first)
                        is_first = false;
                    else
                        oss << ',';
                }
                oss << '}';
                return oss.str();
            }

            string_t pretty_serialize(size_t depth = 0, char table_ch = '\t') const noexcept override
            {
                std::ostringstream oss;
                oss << '{' << '\n';
                bool is_first = true;
                for (const auto &[k, v] : m_obj)
                {
                    if (is_first)
                        is_first = false;
                    else
                        oss << ',' << '\n';
                    for (size_t idx = 0; idx <= depth; ++idx)
                        oss << table_ch;
                    oss << '\"' << k << '\"' << ':';
                    if (!(v->is_value()))
                    {
                        oss << '\n';
                        for (size_t idx = 0; idx <= depth; ++idx)
                            oss << table_ch;
                    }
                    oss << v->pretty_serialize(depth + 1, table_ch);
                }

                oss << '\n';
                for (size_t idx = 0; idx < depth; ++idx)
                    oss << table_ch;
                oss << '}';
                return oss.str();
            }

        public:
            bool operator==(const Object &other) const noexcept
            {
                if (this == &other)
                    return true;
                return m_obj.size() == other.m_obj.size() &&
                       std::equal(m_obj.begin(), m_obj.end(), other.m_obj.begin(),
                                  [](
                                      const std::pair<string_t, Element *> &a,
                                      const std::pair<string_t, Element *> b)
                                  { return a.second && b.second && *(a.second) == *(b.second); });
            }

            bool operator!=(const Object &other) const noexcept { return !((*this) == other); }

            bool operator==(const Element &other) const noexcept override
            {
                if (const Object *arr = dynamic_cast<const Object *>(&other))
                    return (*this) == (*arr);
                return false;
            }

            bool operator!=(const Element &other) const noexcept override { return !((*this) == other); }

        public:
            size_t size() const noexcept { return m_obj.size(); }
            bool empty() const noexcept { return m_obj.empty(); }
            bool contains(const string_t &p_key) const noexcept { return m_obj.find(p_key) != m_obj.end(); }

        public:
            Element *operator[](const string_t &p_key)
            {
                auto it = m_obj.find(p_key);
                return (it != m_obj.end()) ? it->second : nullptr;
            }

            const Element *operator[](const string_t &p_key) const
            {
                auto it = m_obj.find(p_key);
                return (it != m_obj.end()) ? it->second : nullptr;
            }

            Element *get(const string_t &p_key)
            {
                auto ret = operator[](p_key);
                if (ret != nullptr)
                    return ret;
                else
                    throw InvalidKeyException("invalid key!");
            }

            const Element *get(const string_t &p_key) const
            {
                auto ret = operator[](p_key);
                if (ret != nullptr)
                    return ret;
                else
                    throw InvalidKeyException("invalid key!");
            }

        public:
            void insert_raw_ptr(const string_t &p_key, Element *child)
            {
                auto it = m_obj.find(p_key);
                if (it != m_obj.end())
                    delete it->second;
                m_obj[p_key] = child;
            }
            void insert_all_raw_ptr(const object_t<Element *> &other)
            {
                for (auto &child : other)
                    insert_raw_ptr(child.first, child.second);
            }

            void copy_and_insert(const string_t &property, const Element &child) { insert_raw_ptr(property, child.copy()); }
            void copy_and_insert_all(const object_t<Element *> &other) noexcept
            {
                for (const auto &child : other)
                    copy_and_insert(child.first, *(child.second));
            }

            void insert(const string_t &p_key, bool p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
            void insert(const string_t &p_key, int p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
            void insert(const string_t &p_key, float p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
            void insert(const string_t &p_key, const string_t &p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
            void insert(const string_t &p_key, char *p_value) { insert_raw_ptr(p_key, new Value(p_value)); }

            void *operator new(size_t n) { Object::pool.allocate(n); }

            void operator delete(void *ptr) { Object::pool.deallocate(ptr); }
        };

        ObjectPool<Object> Object::pool;
    }
}

#endif // INCLUDE_JSON_OBJECT