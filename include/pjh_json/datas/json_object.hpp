#ifndef INCLUDE_JSON_OBJECT
#define INCLUDE_JSON_OBJECT

#include <algorithm>

#include <pjh_json/helpers/json_definition.hpp>
#include <pjh_json/helpers/json_exception.hpp>

#include <pjh_json/datas/json_element.hpp>
#include <pjh_json/datas/json_value.hpp>

#include <pjh_json/utils/object_pool.hpp>

namespace pjh_std
{
    namespace json
    {
        /**
         * @class Object
         * @brief 表示一个 JSON 对象，即键值对的集合。
         */
        class Object : public Element
        {
        private:
            object_t<Element *> m_obj;      // 使用哈希表存储键和指向 Element 的指针
            static ObjectPool<Object> pool; // 用于 Object 对象的静态对象池

        public:
            /// @brief 默认构造函数，创建空对象。
            Object() { m_obj.clear(); };
            /// @brief 构造函数，从一个键值对 map 创建对象（转移所有权）。
            Object(const object_t<Element *> &val) noexcept { insert_all_raw_ptr(val); }

            /// @brief 拷贝构造函数，深拷贝另一个 Object。
            Object(const Object &other) noexcept { copy_and_insert_all(other.m_obj); }
            Object(const Object *other) noexcept { copy_and_insert_all(other->m_obj); }
            /// @brief 移动构造函数。
            Object(Object &&other) noexcept : m_obj(std::move(other.m_obj)) {}

            /// @brief 析构函数，会调用 clear() 来释放所有子元素的内存。
            ~Object() override { clear(); }

        public:
            /// @brief 覆写基类方法，表明这是一个 Object 类型。
            bool is_object() const noexcept override { return true; }
            /// @brief 覆写基类方法，返回 this 指针。
            Object *as_object() override { return this; }
            /// @brief 返回底层的键值对 map。
            object_t<Element *> as_raw_ptr_map() const noexcept { return m_obj; }

        public:
            /// @brief 清空对象，并递归删除所有子元素，释放内存。
            void clear() override
            {
                for (auto it = m_obj.begin(); it != m_obj.end(); ++it)
                    (it->second)->clear(), delete it->second;
                m_obj.clear();
            }

            /// @brief 创建并返回当前 Object 对象的深拷贝。
            Object *copy() const noexcept override { return new Object(this); }

            /// @brief 将 Object 序列化为紧凑的 JSON 字符串。
            string_t serialize() const noexcept override
            {
                std::ostringstream oss;
                oss << '{';
                bool is_first = true;
                for (const auto &[k, v] : m_obj)
                {
                    if (is_first)
                        is_first = false;
                    else
                        oss << ',';
                    oss << '\"' << k << '\"' << ':' << v->serialize();
                }
                oss << '}';
                return oss.str();
            }

            /// @brief 将 Object 序列化为带缩进的美化 JSON 字符串。
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
                    // 添加当前层级的缩进
                    for (size_t idx = 0; idx <= depth; ++idx)
                        oss << table_ch;
                    // 序列化键
                    oss << '\"' << k << '\"' << ':';
                    // 如果值不是简单类型，则换行并再次缩进
                    if (!(v->is_value()))
                    {
                        oss << '\n';
                        for (size_t idx = 0; idx <= depth; ++idx)
                            oss << table_ch;
                    }
                    // 递归序列化值，并增加深度
                    oss << v->pretty_serialize(depth + 1, table_ch);
                }

                oss << '\n';
                // 添加闭合括号前的缩进
                for (size_t idx = 0; idx < depth; ++idx)
                    oss << table_ch;
                oss << '}';
                return oss.str();
            }

        public:
            /// @brief 比较两个 Object 对象是否相等。
            bool operator==(const Object &other) const noexcept
            {
                if (this == &other)
                    return true;
                return m_obj.size() == other.m_obj.size() &&
                       std::equal(m_obj.begin(), m_obj.end(), other.m_obj.begin(),
                                  [](
                                      const std::pair<string_v_t, Element *> &a,
                                      const std::pair<string_v_t, Element *> b)
                                  { return a.second && b.second && *(a.second) == *(b.second); });
            }
            bool operator!=(const Object &other) const noexcept { return !((*this) == other); }

            /// @brief 比较 Object 和 Element 对象是否相等。
            bool operator==(const Element &other) const noexcept override
            {
                if (const Object *arr = dynamic_cast<const Object *>(&other))
                    return (*this) == (*arr);
                return false;
            }
            bool operator!=(const Element &other) const noexcept override { return !((*this) == other); }

        public:
            /// @brief 返回对象中的键值对数量。
            size_t size() const noexcept { return m_obj.size(); }
            /// @brief 检查对象是否为空。
            bool empty() const noexcept { return m_obj.empty(); }
            /// @brief 检查对象是否包含指定的键。
            bool contains(string_v_t p_key) const noexcept { return m_obj.find(p_key) != m_obj.end(); }

        public:
            /// @brief 通过键访问元素，不进行检查，若键不存在则返回 nullptr。
            Element *operator[](string_v_t p_key)
            {
                auto it = m_obj.find(p_key);
                return (it != m_obj.end()) ? it->second : nullptr;
            }
            const Element *operator[](string_v_t p_key) const
            {
                auto it = m_obj.find(p_key);
                return (it != m_obj.end()) ? it->second : nullptr;
            }

            /// @brief 通过键访问元素，若键不存在则抛出异常。
            Element *get(string_v_t p_key)
            {
                auto ret = operator[](p_key);
                if (ret != nullptr)
                    return ret;
                else
                    throw InvalidKeyException("invalid key!");
            }
            const Element *get(string_v_t p_key) const
            {
                auto ret = operator[](p_key);
                if (ret != nullptr)
                    return ret;
                else
                    throw InvalidKeyException("invalid key!");
            }

        public:
            /// @brief 插入一个键值对（转移所有权），如果键已存在则会替换并删除旧值。
            void insert_raw_ptr(const string_v_t &p_key, Element *child)
            {
                auto it = m_obj.find(p_key);
                if (it != m_obj.end())
                    delete it->second;
                m_obj[p_key] = child;
            }
            /// @brief 插入多个键值对（转移所有权）。
            void insert_all_raw_ptr(const object_t<Element *> &other)
            {
                for (auto &child : other)
                    insert_raw_ptr(child.first, child.second);
            }

            /// @brief 插入一个键值对（拷贝值）。
            void copy_and_insert(const string_v_t &property, const Element &child) { insert_raw_ptr(property, child.copy()); }
            /// @brief 插入多个键值对（拷贝值）。
            void copy_and_insert_all(const object_t<Element *> &other) noexcept
            {
                for (const auto &child : other)
                    copy_and_insert(child.first, *(child.second));
            }

            /// @brief 插入各种基础类型值的便捷方法。
            void insert(const string_t &p_key, bool p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
            void insert(const string_t &p_key, int p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
            void insert(const string_t &p_key, float p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
            void insert(const string_t &p_key, const char *p_value) { insert_raw_ptr(p_key, new Value(string_t(p_value))); }
            void insert(const string_t &p_key, const string_t &p_value) { insert_raw_ptr(p_key, new Value(p_value)); }

            /// @brief 重载 new 运算符，使用对象池进行内存分配。
            void *operator new(std::size_t n) { return Object::pool.allocate(n); }
            /// @brief 重载 delete 运算符，将内存归还给对象池。
            void operator delete(void *ptr) { Object::pool.deallocate(ptr); }
        };
        // 静态成员初始化
        inline ObjectPool<Object> Object::pool;
    }
}

#endif // INCLUDE_JSON_OBJECT