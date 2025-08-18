#ifndef INCLUDE_JSON_ARRAY
#define INCLUDE_JSON_ARRAY

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
        /**
         * @class Array
         * @brief 表示一个 JSON 数组，可以包含任意类型的 JSON Element。
         */
        class Array : public Element
        {
        private:
            array_t<Element *> m_arr;      // 使用 vector 存储指向 Element 的指针
            static ObjectPool<Array> pool; // 用于 Array 对象的静态对象池

        public:
            /// @brief 默认构造函数，创建空数组。
            Array() : m_arr() {}
            /// @brief 构造函数，预分配指定大小的空间。
            Array(const size_t count) : m_arr(count) {}
            /// @brief 构造函数，从一个元素指针的 vector 创建数组（转移所有权）。
            Array(const array_t<Element *> &val) noexcept { append_all_raw_ptr(val); }

            /// @brief 拷贝构造函数，深拷贝另一个 Array。
            Array(const Array &other) { copy_and_append_all(other.as_vector()); }
            Array(const Array *other) { copy_and_append_all(other->as_vector()); }
            /// @brief 移动构造函数。
            Array(Array &&other) noexcept : m_arr(std::move(other.m_arr)) {}

            /// @brief 拷贝赋值运算符。
            Array &operator=(const Array &other)
            {
                if (this != &other)
                    this->m_arr = other.m_arr;
                return *this;
            }

            /// @brief 移动赋值运算符。
            Array &operator=(Array &&other)
            {
                if (this != &other)
                    this->m_arr = std::move(other.m_arr);
                return *this;
            }

            /// @brief 从 Element 基类进行赋值。
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

            /// @brief 析构函数，会调用 clear() 来释放所有子元素的内存。
            ~Array() override { clear(); }

        public:
            /// @brief 覆写基类方法，表明这是一个 Array 类型。
            bool is_array() const noexcept override { return true; }
            /// @brief 覆写基类方法，返回 this 指针。
            Array *as_array() override { return this; }

            /// @brief 如果数组只有一个元素，则返回该元素，否则返回 nullptr。
            Element *as_element() const noexcept { return size() == 1 ? m_arr[0] : nullptr; }
            /// @brief 返回底层的元素指针 vector。
            array_t<Element *> as_vector() const noexcept { return m_arr; }

        public:
            /// @brief 清空数组，并递归删除所有子元素，释放内存。
            void clear() override
            {
                for (const auto &it : m_arr)
                    if (it != nullptr)
                        it->clear(), delete it;
                m_arr.clear();
            }

            /// @brief 创建并返回当前 Array 对象的深拷贝。
            Element *copy() const noexcept override { return new Array(this); }

            /// @brief 将 Array 序列化为紧凑的 JSON 字符串。
            string_t serialize() const noexcept override
            {
                std::ostringstream oss;
                oss << '[';
                bool is_first = true;
                for (const auto &it : m_arr)
                {
                    if (is_first)
                        is_first = false;
                    else
                        oss << ',';
                    oss << it->serialize();
                }
                oss << ']';
                return oss.str();
            }

            /// @brief 将 Array 序列化为带缩进的美化 JSON 字符串。
            string_t pretty_serialize(size_t depth = 0, char table_ch = 't') const noexcept override
            {
                std::ostringstream oss;
                oss << '[' << '\n';
                bool is_first = true;
                for (const auto &it : m_arr)
                {
                    if (is_first)
                        is_first = false;
                    else
                        oss << ',' << '\n';

                    // 添加当前层级的缩进
                    for (size_t idx = 0; idx <= depth; ++idx)
                        oss << table_ch;
                    // 递归序列化子元素，并增加深度
                    oss << it->pretty_serialize(depth + 1, table_ch);
                }
                oss << '\n';
                // 添加闭合括号前的缩进
                for (size_t idx = 0; idx < depth; ++idx)
                    oss << table_ch;
                oss << ']';
                return oss.str();
            }

        public:
            /// @brief 比较两个 Array 对象是否相等。
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

            /// @brief 比较 Array 和 Element 对象是否相等。
            bool operator==(const Element &other) const noexcept override
            {
                if (const Array *arr = dynamic_cast<const Array *>(&other))
                    return (*this) == (*arr);
                return false;
            }
            bool operator!=(const Element &other) const noexcept override { return !((*this) == other); }

        public:
            /// @brief 返回数组中的元素数量。
            size_t size() const noexcept { return m_arr.size(); }
            /// @brief 检查数组是否为空。
            bool empty() const noexcept { return m_arr.empty(); }
            /// @brief 检查数组是否包含指定的子元素。
            bool contains(Element *p_child) { return std::find(m_arr.begin(), m_arr.end(), p_child) != m_arr.end(); }

        public:
            /// @brief 通过索引访问元素，不进行边界检查。
            Element *operator[](size_t p_index) noexcept { return p_index < size() ? m_arr[p_index] : nullptr; }
            const Element *operator[](size_t p_index) const noexcept { return p_index < size() ? m_arr[p_index] : nullptr; }

            /// @brief 通过索引访问元素，进行边界检查，越界则抛出异常。
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
            /// @brief 设置指定索引处的元素（转移所有权），如果旧元素存在则会删除。
            void set_raw_ptr(size_t idx, Element *child)
            {
                if (size() <= idx)
                    m_arr.resize(idx);
                if (m_arr[idx] != nullptr)
                    delete m_arr[idx];
                m_arr[idx] = child;
            }

            /// @brief 在数组末尾添加一个元素（转移所有权）。
            void append_raw_ptr(Element *child) { m_arr.push_back(child); }
            /// @brief 在数组末尾添加多个元素（转移所有权）。
            void append_all_raw_ptr(const std::vector<Element *> &children)
            {
                for (auto &child : children)
                    append_raw_ptr(child);
            }

            /// @brief 在数组末尾添加一个元素的拷贝。
            void copy_and_append(const Element &child) { append_raw_ptr(child.copy()); }
            /// @brief 在数组末尾添加多个元素的拷贝。
            void copy_and_append_all(const std::vector<Element *> &children)
            {
                for (auto &child : children)
                    copy_and_append(*child);
            }

            /// @brief 添加各种基础类型值的便捷方法。
            void append(bool p_value) { append_raw_ptr(new Value(p_value)); }
            void append(int p_value) { append_raw_ptr(new Value(p_value)); }
            void append(float p_value) { append_raw_ptr(new Value(p_value)); }
            void append(const string_t &p_value) { append_raw_ptr(new Value(p_value)); }
            void append(char *p_value) { append_raw_ptr(new Value(p_value)); }

        public:
            /// @brief 删除指定索引处的元素。
            void erase(const size_t &idx) { m_arr.erase(m_arr.begin() + idx); }

            /// @brief 删除指定的子元素。
            void remove(const Element *child)
            {
                if (child == nullptr)
                    return;
                auto it = std::find(m_arr.begin(), m_arr.end(), child);
                if (it != m_arr.end())
                    m_arr.erase(it);
            }
            /// @brief 删除多个指定的子元素。
            void remove_all(const std::vector<Element *> &children)
            {
                for (const Element *child : children)
                    remove(child);
            }

            /// @brief 重载 new 运算符，使用对象池进行内存分配。
            void *operator new(std::size_t n) { return Array::pool.allocate(n); }
            /// @brief 重载 delete 运算符，将内存归还给对象池。
            void operator delete(void *ptr) { Array::pool.deallocate(ptr); }
        };
        // 静态成员初始化
        inline ObjectPool<Array> Array::pool;
    }
}

#endif // INCLUDE_JSON_ARRAY