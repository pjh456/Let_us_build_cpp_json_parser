#ifndef INCLUDE_JSON
#define INCLUDE_JSON

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <stdexcept>
#include <algorithm>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <cstdint>
#include <utility>
#include <thread>
#include <atomic>
#include <optional>
#include <chrono>
#include <string_view>
#include <charconv>

namespace pjh_std
{
    namespace json
    {
        // Json values
        class Ref;
        class Element;

        class Value;
        class Object;
        class Array;

        using string_t = std::string;
        using string_v_t = std::string_view;

        using value_t = std::variant<
            std::nullptr_t,
            bool,
            int,
            float,
            string_t,
            string_v_t>;

        template <typename T>
        using array_t = std::vector<T>;

        struct StringViewHash
        {
            using is_transparent = void; // 关键！启用透明模式

            size_t operator()(std::string_view sv) const
            {
                return std::hash<std::string_view>{}(sv);
            }
        };

        template <typename T>
        using object_t = std::unordered_map<
            string_v_t,
            T,
            StringViewHash,
            std::equal_to<>>;

        // Exceptions
        class Exception;

        class ParseException;
        class TypeException;
        class OutOfRangeException;
        class InvalidKeyException;
        class SerializationException;

        // Base Json Exception
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

        class ThreadException : public Exception
        {
        public:
            explicit ThreadException(const std::string &msg)
                : Exception("Thread error: " + msg) {}
        };

        template <typename T>
        class MallocAllocator
        {
        public:
            T *allocate(size_t n)
            {
                void *raw = ::malloc(n);
                if (!raw)
                    throw std::bad_alloc();
                return reinterpret_cast<T *>(raw);
            }

            void deallocate(T *ptr)
            {
                ::free(ptr);
            }
        };

        template <typename T>
        class FreeListAllocator
        {
            struct Node
            {
                Node *next;
            };
            Node *freeList = nullptr;

        public:
            ~FreeListAllocator()
            {
                while (freeList)
                {
                    Node *tmp = freeList;
                    freeList = freeList->next;
                    ::free(tmp);
                }
            }

            T *allocate(size_t n)
            {
                if (freeList)
                {
                    Node *node = freeList;
                    freeList = node->next;
                    return reinterpret_cast<T *>(node);
                }
                void *raw = ::malloc(n);
                if (!raw)
                    throw std::bad_alloc();
                return reinterpret_cast<T *>(raw);
            }

            void deallocate(T *ptr)
            {
                Node *node = reinterpret_cast<Node *>(ptr);
                node->next = freeList;
                freeList = node;
            }
        };

        template <typename T, size_t BlockSize = 4096>
        class BlockAllocator
        {
            std::vector<void *> blocks;
            T *currentBlock = nullptr;
            size_t remaining = 0;

        public:
            T *allocate(size_t n)
            {
                if (remaining == 0)
                {
                    // 分配新块
                    void *block = ::malloc(sizeof(T) * BlockSize);
                    if (!block)
                        throw std::bad_alloc();
                    blocks.push_back(block);
                    currentBlock = reinterpret_cast<T *>(block);
                    remaining = BlockSize;
                }
                T *obj = currentBlock++;
                --remaining;
                return obj;
            }

            void deallocate(T *ptr)
            {
                // 对 BlockAllocator 通常不立即回收单个对象，等析构时释放
            }

            ~BlockAllocator()
            {
                for (void *block : blocks)
                    ::free(block);
            }
        };

        template <typename T, typename Alloc = BlockAllocator<T>>
        class ObjectPool
        {
        private:
            Alloc m_allocator;

        public:
            ObjectPool() = default;
            ~ObjectPool() = default;

        public:
            T *allocate(size_t n) { return m_allocator.allocate(n); }

            void deallocate(void *ptr) { m_allocator.deallocate(static_cast<T *>(ptr)); }
        };
        template <typename T>
        class Channel
        {
        private:
            size_t m_capacity;
            std::queue<T> m_queue;
            std::mutex m_mutex;
            std::condition_variable m_cond_not_empty;
            std::condition_variable m_cond_not_full;

        public:
            Channel(size_t p_capacity = 0) : m_capacity(p_capacity) {}

            ~Channel() = default;

        public:
            void push(const T &p_item)
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cond_not_full.wait(
                    lock, [&]()
                    { return m_capacity == 0 || m_queue.size() < m_capacity; });
                m_queue.push(p_item);
                lock.unlock();
                m_cond_not_empty.notify_one();
            }

            void push(T &&p_item)
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cond_not_full.wait(
                    lock, [&]()
                    { return m_capacity == 0 || m_queue.size() < m_capacity; });
                m_queue.push(std::move(p_item));
                lock.unlock();
                m_cond_not_empty.notify_one();
            }

            void pop()
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cond_not_empty.wait(
                    lock, [&]()
                    { return !m_queue.empty(); });
                m_queue.pop();
                lock.unlock();
                m_cond_not_full.notify_one();
            }

            T peek()
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cond_not_empty.wait(
                    lock, [&]()
                    { return !m_queue.empty(); });
                T item = std::move(m_queue.front());
                lock.unlock();
                return item;
            }
        };

        template <typename T>
        class LockFreeRingBuffer
        {
        private:
            size_t m_capacity;
            std::vector<T> m_buffer;
            std::atomic<size_t> m_head{0};
            std::atomic<size_t> m_tail{0};

        public:
            LockFreeRingBuffer(size_t p_capacity = 256)
                : m_capacity(p_capacity), m_buffer(p_capacity) {}
            ~LockFreeRingBuffer() = default;

        public:
            bool push(const T &p_item)
            {
                size_t tail = m_tail.load(std::memory_order_relaxed);
                size_t next_tail = (tail + 1) % m_capacity;
                if (next_tail == m_head.load(std::memory_order_acquire))
                    return false; // Full
                m_buffer[tail] = p_item;
                m_tail.store(next_tail, std::memory_order_release);
                return true;
            }

            bool pop()
            {
                size_t head = m_head.load(std::memory_order_relaxed);
                size_t next_head = (head + 1) % m_capacity;
                if (head == m_tail.load(std::memory_order_acquire))
                    return false;
                m_head.store(next_head, std::memory_order_release);
                return true;
            }

            std::optional<T> peek()
            {
                size_t head = m_head.load(std::memory_order_relaxed);
                if (head == m_tail.load(std::memory_order_acquire))
                    return std::nullopt;
                T value = m_buffer[head];
                return value;
            }
        };

        class Element
        {
        public:
            virtual ~Element() {}

            virtual bool is_value() const noexcept { return false; }
            virtual bool is_array() const noexcept { return false; }
            virtual bool is_object() const noexcept { return false; }

            virtual Value *as_value() { throw TypeException("Invalid base type!"); }
            virtual Array *as_array() { throw TypeException("Invalid base type!"); }
            virtual Object *as_object() { throw TypeException("Invalid base type!"); }

            virtual void clear() {}
            virtual Element *copy() const = 0;
            virtual string_t serialize() const noexcept { return ""; }
            virtual string_t pretty_serialize(size_t = 0, char = '\t') const noexcept { return ""; }

            virtual bool operator==(const Element &other) const noexcept { return false; }
            virtual bool operator!=(const Element &other) const noexcept { return true; }
        };

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

            Value(const char *p_value) : m_value(string_t(p_value)) {}
            Value(const string_t &p_value) : m_value(p_value) {}
            Value(string_t &&p_value) : m_value(std::move(p_value)) {}
            Value(std::string_view p_value) : m_value(p_value) {}

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

            Value &operator=(Element &&other)
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
            bool is_str() const noexcept { return is_T<string_t>() || is_T<string_v_t>(); }

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

            void *operator new(std::size_t n) { return Value::pool.allocate(n); }

            void operator delete(void *ptr) { Value::pool.deallocate(ptr); }
        };

        inline ObjectPool<Value> Value::pool;

        class Array : public Element
        {
        private:
            array_t<Element *> m_arr;
            static ObjectPool<Array> pool;

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

                    for (size_t idx = 0; idx <= depth; ++idx)
                        oss << table_ch;
                    oss << it->pretty_serialize(depth + 1, table_ch);
                }
                oss << '\n';
                for (size_t idx = 0; idx < depth; ++idx)
                    oss << table_ch;
                oss << ']';
                return oss.str();
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

            void *operator new(std::size_t n) { return Array::pool.allocate(n); }

            void operator delete(void *ptr) { Array::pool.deallocate(ptr); }
        };

        inline ObjectPool<Array> Array::pool;

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
                                      const std::pair<string_v_t, Element *> &a,
                                      const std::pair<string_v_t, Element *> b)
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
            bool contains(string_v_t p_key) const noexcept { return m_obj.find(p_key) != m_obj.end(); }

        public:
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
            void insert_raw_ptr(const string_v_t &p_key, Element *child)
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

            void copy_and_insert(const string_v_t &property, const Element &child) { insert_raw_ptr(property, child.copy()); }
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

            void *operator new(std::size_t n) { return Object::pool.allocate(n); }

            void operator delete(void *ptr) { Object::pool.deallocate(ptr); }
        };

        inline ObjectPool<Object> Object::pool;

        class Ref
        {
        private:
            Element *m_ptr;

        public:
            Ref(Element *p_ptr = nullptr) : m_ptr(p_ptr) {}

            Ref operator[](string_v_t p_key)
            {
                if (!m_ptr)
                    throw NullPointerException("Null reference");
                if (auto obj = m_ptr->as_object())
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

            Element *get() const { return m_ptr; }

        public:
            friend std::ostream &operator<<(std::ostream &os, Ref &ref)
            {
                os << ref.get()->pretty_serialize(0, ' ');
                return os;
            }
        };

        Ref make_object(std::initializer_list<std::pair<string_t, Ref>> p_list)
        {
            auto obj = new Object();
            for (auto &kv : p_list)
                obj->insert_raw_ptr(kv.first, kv.second.get());
            return Ref(obj);
        }

        Ref make_array(std::initializer_list<Ref> p_list)
        {
            auto arr = new Array();
            for (const auto &v : p_list)
                arr->append_raw_ptr(v.get());
            return Ref(arr);
        }

        Ref make_value(std::nullptr_t) { return Ref(new Value(nullptr)); }
        Ref make_value(bool p_val) { return Ref(new Value(p_val)); }
        Ref make_value(int p_val) { return Ref(new Value(p_val)); }
        Ref make_value(float p_val) { return Ref(new Value(p_val)); }
        Ref make_value(const string_t p_val) { return Ref(new Value(p_val)); }
        Ref make_value(char *p_val) { return Ref(new Value((string_t)p_val)); }

        enum class TokenType
        {
            ObjectBegin, // '{'
            ObjectEnd,   // '}'
            ArrayBegin,  // '['
            ArrayEnd,    // ']'
            Colon,       // ':'
            Comma,       // ','
            String,      // "string"
            Integer,     // 123
            Float,       // 12.72
            Bool,        // true or false
            Null,        // null
            End          // End of input
        };

        struct Token
        {
            TokenType type;
            std::string_view value;
        };

        class Tokenizer
        {
        private:
            std::string m_str;
            size_t m_pos;

            size_t line;
            size_t column;

            Token m_current_token;

        public:
            // Constructor
            Tokenizer(const std::string &p_str)
                : m_str(p_str), m_pos(0),
                  line(1), column(1) { consume(); }
            Tokenizer(const Tokenizer &other) = default;
            ~Tokenizer() = default;

            // Token Getter
            const Token peek() const noexcept { return m_current_token; }

            void consume() { m_current_token = read_next_token(); }

        private:
            bool eof() const noexcept { return m_pos >= m_str.size(); }
            char peek_char() const { return m_str[m_pos]; }
            char get_char() { return column++, m_str[m_pos++]; }

            Token read_next_token()
            {
                skip_white_space();
                if (eof())
                    return {TokenType::End, ""};

                const size_t start_pos = m_pos;
                char current_ch = peek_char();

                switch (current_ch)
                {
                case '{':
                    get_char();
                    return {TokenType::ObjectBegin, std::string_view(m_str.data() + start_pos, 1)};
                case '}':
                    get_char();
                    return {TokenType::ObjectEnd, std::string_view(m_str.data() + start_pos, 1)};
                case '[':
                    get_char();
                    return {TokenType::ArrayBegin, std::string_view(m_str.data() + start_pos, 1)};
                case ']':
                    get_char();
                    return {TokenType::ArrayEnd, std::string_view(m_str.data() + start_pos, 1)};
                case ':':
                    get_char();
                    return {TokenType::Colon, std::string_view(m_str.data() + start_pos, 1)};
                case ',':
                    get_char();
                    return {TokenType::Comma, std::string_view(m_str.data() + start_pos, 1)};
                case '"':
                    return parse_string();
                case 't':
                case 'f':
                    return parse_bool();
                case 'n':
                    return parse_null();
                default:
                    if (isdigit(current_ch) || current_ch == '-')
                        return parse_number();
                    else
                        throw ParseException(line, column, (std::string) "Unexpected character '" + std::string(1, current_ch) + "'");
                    break;
                }
            }

        private:
            // Skip white space. May have a faster way?
            inline void skip_white_space()
            {
                while (!eof() && (peek_char() == '\n' || peek_char() == '\t' || peek_char() == ' '))
                {
                    if (get_char() == '\n')
                        line++, column = 1;
                }
            }

            // Parse Number Token
            Token parse_number()
            {
                const size_t start_pos = m_pos;
                bool is_float = false;

                if (!eof() && peek_char() == '-')
                    get_char();

                while (!eof() && isdigit(peek_char()))
                    get_char();

                if (!eof() && peek_char() == '.')
                {
                    is_float = true;
                    get_char();

                    while (!eof() && isdigit(peek_char()))
                        get_char();
                }

                return {
                    is_float ? TokenType::Float : TokenType::Integer,
                    std::string_view(m_str.data() + start_pos, m_pos - start_pos)};
            }

            Token parse_bool()
            {
                using namespace std::literals::string_view_literals;

                const size_t start_pos = m_pos;
                if (m_str.size() - start_pos >= 4 && std::string_view(m_str.data() + start_pos, 4) == "true"sv)
                {
                    m_pos += 4;
                    column += 4;
                    return {TokenType::Bool, std::string_view(m_str.data() + start_pos, 4)};
                }
                if (m_str.size() - start_pos >= 5 && std::string_view(m_str.data() + start_pos, 5) == "false"sv)
                {
                    m_pos += 5;
                    column += 5;
                    return {TokenType::Bool, std::string_view(m_str.data() + start_pos, 5)};
                }
                throw ParseException(line, column, "Invalid boolean literal");
            }

            Token parse_string()
            {
                get_char(); // 消费开头的引号 "
                const size_t start_pos_content = m_pos;
                while (!eof())
                {
                    char ch = peek_char(); // 只查看，不消费
                    if (ch == '\\')
                    {
                        get_char(); // 消费 '\'
                        if (!eof())
                            get_char(); // 消费被转义的字符
                    }
                    else if (ch == '"') // 字符串结束
                    {
                        size_t end_pos_content = m_pos;
                        get_char(); // 消费结尾的引号 "
                        return {TokenType::String, std::string_view(m_str.data() + start_pos_content, end_pos_content - start_pos_content)};
                    }
                    else
                        get_char(); // 消费普通字符
                }
                throw ParseException(line, column, "Unterminated string literal");
            }

            Token parse_null()
            {
                using namespace std::literals::string_view_literals;

                const size_t start_pos = m_pos;
                if (m_str.size() - start_pos >= 4 && std::string_view(m_str.data() + start_pos, 4) == "null"sv)
                {
                    m_pos += 4;
                    column += 4;
                    // 使用清晰的 start_pos
                    return {TokenType::Null, std::string_view(m_str.data() + start_pos, 4)};
                }
                throw ParseException(line, column, "Invalid null literal");
            }
        };

        class Parser
        {
        private:
            // LockFreeRingBuffer<Token> m_buffer;
            Tokenizer m_tokenizer;

        public:
            Parser(const std::string &p_str, size_t capacity = 16384)
                : m_tokenizer(p_str) /*, m_buffer(capacity)*/ {}

            Parser(Tokenizer &p_tokenizer, size_t capacity)
                : m_tokenizer(p_tokenizer) /*, m_buffer(capacity)*/ {}

            Ref parse() { return Ref(parse_value()); }

            // Ref parse()
            // {
            //     std::thread producer_thread(
            //         [&]()
            //         {
            //             try
            //             {
            //                 while (true)
            //                 {
            //                     Token token = m_tokenizer.peek();
            //                     if (token.type == TokenType::End)
            //                         break;
            //                     while (!m_buffer.push(token))
            //                     {
            //                         std::this_thread::sleep_for(std::chrono::microseconds(10));
            //                         // std::cout << "full!" << std::endl;
            //                     }
            //                     m_tokenizer.consume();
            //                 }
            //             }
            //             catch (const std::exception &e)
            //             {
            //                 // std::cout << e.what() << std::endl;
            //                 throw ThreadException(e.what());
            //             }
            //         });

            //     Ref root = Ref(parse_value());
            //     producer_thread.join();
            //     return root;
            // }

        private:
            Token peek()
            {
                // std::optional<Token> token;
                // while (!(token = m_buffer.peek()).has_value())
                // {
                //     std::this_thread::sleep_for(std::chrono::microseconds(10));
                //     std::cout << "empty!" << std::endl;
                // }
                // return token.value();
                return m_tokenizer.peek();
            }
            void consume()
            {
                // while (!m_buffer.pop())
                // {
                //     std::this_thread::sleep_for(std::chrono::microseconds(10));
                //     std::cout << "empty!" << std::endl;
                // }
                m_tokenizer.consume();
            }

            Element *parse_value()
            {
                Token token = peek();

                switch (token.type)
                {
                case TokenType::ObjectBegin:
                    return Parser::parse_object();
                case TokenType::ArrayBegin:
                    return Parser::parse_array();
                case TokenType::Integer:
                {
                    int val;
                    auto [ptr, ec] = std::from_chars(token.value.data(), token.value.data() + token.value.size(), val);
                    if (ec != std::errc())
                    {
                        throw Exception("Invalid integer: " + std::string(token.value));
                    }
                    consume();
                    return new Value(val);
                }
                case TokenType::Float:
                {
                    // 对于 float，from_chars 的支持可能不完善，一个安全的方式是构造一个临时string
                    try
                    {
                        // stod 需要一个 owning 的 string
                        std::string temp_str(token.value);
                        float val = std::stof(temp_str);
                        consume();
                        return new Value(val);
                    }
                    catch (...)
                    {
                        throw Exception("Invalid float: " + std::string(token.value));
                    }
                }
                case TokenType::Bool:
                {
                    bool val = token.value[0] == 't';
                    consume();
                    return new Value(val);
                }
                case TokenType::String:
                {
                    auto val = token.value;
                    consume();
                    return new Value(val);
                }
                case TokenType::Null:
                    consume();
                    return new Value();
                default:
                    throw TypeException("Unexpected token type");
                }
            }

            Object *parse_object()
            {
                consume();

                Object *obj = new Object();

                if (peek().type == TokenType::ObjectEnd)
                {
                    consume();
                    return obj;
                }

                while (true)
                {
                    Token key_token = peek();

                    if (key_token.type != TokenType::String)
                        throw Exception("Expected string key in object!");
                    auto key = key_token.value;
                    consume();

                    if (peek().type != TokenType::Colon)
                        throw Exception("Expected colon after key!");
                    consume();

                    obj->insert_raw_ptr(key, parse_value());

                    Token next_token = peek();
                    if (next_token.type == TokenType::ObjectEnd)
                    {
                        consume();
                        break;
                    }
                    else if (next_token.type == TokenType::Comma)
                        consume();
                    else
                        throw Exception("Expected ',' or '}' in object");
                }

                return obj;
            }

            Array *parse_array()
            {
                consume();
                Array *arr = new Array();

                if (peek().type == TokenType::ArrayEnd)
                {
                    consume();
                    return arr;
                }

                while (true)
                {
                    arr->append_raw_ptr(parse_value());

                    Token next_token = peek();
                    if (next_token.type == TokenType::ArrayEnd)
                    {
                        consume();
                        break;
                    }
                    else if (next_token.type == TokenType::Comma)
                        consume();
                    else
                        throw Exception("Expected ',' or ']' in array");
                }

                return arr;
            }
        };
    }
}

#endif // INCLUDE_JSON