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

// 定义项目的主命名空间
namespace pjh_std
{
    // 定义 JSON 相关的子命名空间
    namespace json
    {
        // json_definition.hpp
        // 提前声明 JSON 库中的核心类，以解决类之间的循环引用问题
        class Ref;
        class Element;

        class Value;
        class Object;
        class Array;

        // 为 std::string 定义一个更简洁的别名
        using string_t = std::string;
        // 为 std::string_view 定义一个更简洁的别名，用于高效处理字符串切片
        using string_v_t = std::string_view;

        // 使用 std::variant 定义 JSON 的值类型，它可以持有多种不同的基础数据类型
        using value_t = std::variant<
            std::nullptr_t,
            bool,
            int,
            float,
            string_t,
            string_v_t>;

        // JSON 数组的模板别名，底层使用 std::vector
        template <typename T>
        using array_t = std::vector<T>;

        /**
         * @struct StringViewHash
         * @brief 为 std::string_view 提供哈希计算的结构体，用于 unordered_map。
         *        通过启用透明模式(is_transparent)，允许在不创建 std::string 临时对象的情况下，
         *        直接使用 const char* 或 std::string_view 作为键进行查找，提升性能。
         */
        struct StringViewHash
        {
            using is_transparent = void; // 关键！启用透明模式

            size_t operator()(std::string_view sv) const
            {
                return std::hash<std::string_view>{}(sv);
            }
        };

        // JSON 对象的模板别名，底层使用带有自定义哈希的 std::unordered_map
        template <typename T>
        using object_t = std::unordered_map<
            string_v_t,
            T,
            StringViewHash,
            std::equal_to<>>;

        // 提前声明异常类
        class Exception;

        class ParseException;
        class TypeException;
        class OutOfRangeException;
        class InvalidKeyException;
        class SerializationException;

        // 提前声明并发缓冲和内存管理相关类
        template <typename T>
        class Channel;
        template <typename T>
        class LockFreeRingBuffer;
        template <typename T>
        class MallocAllocator;
        template <typename T, size_t BlockSize>
        class BlockAllocator;
        template <typename T>
        class FreeListAllocator;

        template <typename T, typename Allocate>
        class ObjectPool;

        // json_definition.hpp

        // json_exception.hpp
        /**
         * @class Exception
         * @brief 所有 JSON 库异常的基类，继承自 std::runtime_error。
         */
        class Exception : public std::runtime_error
        {
        public:
            /**
             * @brief 构造函数。
             * @param msg 异常信息。
             */
            explicit Exception(const std::string &msg)
                : std::runtime_error(msg) {}
        };

        /**
         * @class ParseException
         * @brief 解析 JSON 字符串时发生语法错误时抛出的异常。
         */
        class ParseException : public Exception
        {
        public:
            /**
             * @brief 构造函数，包含错误发生的行号和列号。
             * @param p_line 错误发生的行号。
             * @param p_col 错误发生的列号。
             * @param msg 具体的错误信息。
             */
            ParseException(size_t p_line, size_t p_col, const std::string &msg)
                : Exception(
                      "Parse error at line " + std::to_string(p_line) +
                      ", column " + std::to_string(p_col) + ": " + msg),
                  m_line(p_line), m_col(p_col) {}

            /// @brief 获取错误行号。
            size_t line() const noexcept { return m_line; }
            /// @brief 获取错误列号。
            size_t column() const noexcept { return m_col; }

        private:
            size_t m_line, m_col; // 存储错误位置的行号和列号
        };

        /**
         * @class TypeException
         * @brief 当尝试以错误的类型访问 JSON 值时抛出的异常。
         */
        class TypeException : public Exception
        {
        public:
            explicit TypeException(const std::string &msg)
                : Exception("Type error: " + msg) {}
        };

        /**
         * @class OutOfRangeException
         * @brief 当访问 JSON 数组的索引超出范围时抛出的异常。
         */
        class OutOfRangeException : public Exception
        {
        public:
            explicit OutOfRangeException(const std::string &msg)
                : Exception("Out of range: " + msg) {}
        };

        /**
         * @class InvalidKeyException
         * @brief 当使用不存在的键访问 JSON 对象时抛出的异常。
         */
        class InvalidKeyException : public Exception
        {
        public:
            explicit InvalidKeyException(const std::string &key)
                : Exception("Invalid key: '" + key + "'") {}
        };

        /**
         * @class SerializationException
         * @brief 在序列化 JSON 对象为字符串时发生错误时抛出的异常。
         */
        class SerializationException : public Exception
        {
        public:
            explicit SerializationException(const std::string &msg)
                : Exception("Serialization error: " + msg) {}
        };

        /**
         * @class NullPointerException
         * @brief 当试图通过空引用或空指针访问 JSON 元素时抛出的异常。
         */
        class NullPointerException : public Exception
        {
        public:
            explicit NullPointerException(const std::string &msg)
                : Exception("Null pointer error: " + msg) {}
        };

        /**
         * @class ThreadException
         * @brief 在多线程操作中发生错误时抛出的异常。
         */
        class ThreadException : public Exception
        {
        public:
            explicit ThreadException(const std::string &msg)
                : Exception("Thread error: " + msg) {}
        };
        // json_exception.hpp

        // object_pool.hpp
        /**
         * @class MallocAllocator
         * @brief 一个简单的基于 malloc/free 的内存分配器。
         */
        template <typename T>
        class MallocAllocator
        {
        public:
            /// @brief 分配内存。
            T *allocate(size_t n)
            {
                void *raw = ::malloc(n);
                if (!raw)
                    throw std::bad_alloc();
                return reinterpret_cast<T *>(raw);
            }
            /// @brief 释放内存。
            void deallocate(T *ptr)
            {
                ::free(ptr);
            }
        };

        /**
         * @class FreeListAllocator
         * @brief 基于自由列表的内存分配器。
         *        它会回收被释放的内存块，用于后续的分配请求，从而减少对系统 `malloc` 的调用。
         */
        template <typename T>
        class FreeListAllocator
        {
            struct Node
            {
                Node *next;
            };
            Node *freeList = nullptr; // 指向可用内存块链表的头指针

        public:
            /// @brief 析构时释放所有在自由列表中的内存块。
            ~FreeListAllocator()
            {
                while (freeList)
                {
                    Node *tmp = freeList;
                    freeList = freeList->next;
                    ::free(tmp);
                }
            }

            /// @brief 分配一个对象所需的内存。优先从自由列表中获取，失败则调用 `malloc`。
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

            /// @brief 回收一个对象的内存，将其加入到自由列表的头部。
            void deallocate(T *ptr)
            {
                Node *node = reinterpret_cast<Node *>(ptr);
                node->next = freeList;
                freeList = node;
            }
        };

        /**
         * @class BlockAllocator
         * @brief 块内存分配器。一次性分配一大块内存（一个 Block），然后从中逐个分配小对象。
         *        这种方式可以减少内存碎片和分配开销，但通常不单独回收对象，而是在析构时统一释放所有块。
         */
        template <typename T, size_t BlockSize = 4096>
        class BlockAllocator
        {
            std::vector<void *> blocks; // 存储所有已分配的大内存块
            T *currentBlock = nullptr;  // 指向当前正在分配的内存块
            size_t remaining = 0;       // 当前内存块中剩余可分配的对象数量

        public:
            /// @brief 分配一个对象所需的内存。
            T *allocate(size_t n)
            {
                // 如果当前块已用完，则分配一个新块
                if (remaining == 0)
                {
                    void *block = ::malloc(sizeof(T) * BlockSize);
                    if (!block)
                        throw std::bad_alloc();
                    blocks.push_back(block);
                    currentBlock = reinterpret_cast<T *>(block);
                    remaining = BlockSize;
                }
                // 从当前块中分配一个对象，并更新指针和剩余计数
                T *obj = currentBlock++;
                --remaining;
                return obj;
            }

            /// @brief 对于块分配器，通常不立即回收单个对象，所以此函数为空。
            void deallocate(T *ptr) {}

            /// @brief 析构函数，释放所有持有的大内存块。
            ~BlockAllocator()
            {
                for (void *block : blocks)
                    ::free(block);
            }
        };

        /**
         * @class ObjectPool
         * @brief 对象池，用于高效地管理特定类型对象的内存分配和回收。
         *        通过自定义分配器（默认为 BlockAllocator）来减少内存分配的开销。
         */
        template <typename T, typename Alloc = BlockAllocator<T>>
        class ObjectPool
        {
        private:
            Alloc m_allocator; // 底层的内存分配器实例

        public:
            ObjectPool() = default;
            ~ObjectPool() = default;

            /// @brief 从对象池中分配一个对象。
            T *allocate(size_t n) { return m_allocator.allocate(n); }
            /// @brief 将一个对象归还给对象池。
            void deallocate(void *ptr) { m_allocator.deallocate(static_cast<T *>(ptr)); }
        };
        // object_pool.hpp

        // channel.hpp
        /**
         * @class Channel
         * @brief 一个线程安全的生产者-消费者通道。
         *        当通道满时，生产者会阻塞；当通道空时，消费者会阻塞。
         */
        template <typename T>
        class Channel
        {
        private:
            size_t m_capacity;                        // 通道容量，为0表示无限制
            std::queue<T> m_queue;                    // 底层队列
            std::mutex m_mutex;                       // 互斥锁，保护队列访问
            std::condition_variable m_cond_not_empty; // 条件变量，用于通知消费者队列非空
            std::condition_variable m_cond_not_full;  // 条件变量，用于通知生产者队列未满

        public:
            /// @brief 构造函数。@param p_capacity 通道容量。
            Channel(size_t p_capacity = 0) : m_capacity(p_capacity) {}
            ~Channel() = default;

        public:
            /// @brief 向通道中推入一个元素（拷贝）。
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

            /// @brief 向通道中推入一个元素（移动）。
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

            /// @brief 从通道中弹出一个元素。
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

            /// @brief 查看通道的头部元素，但不弹出。
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
        // channel.hpp

        // lock_free_ring_buffer.hpp
        /**
         * @class LockFreeRingBuffer
         * @brief 一个无锁环形缓冲区，适用于单生产者-单消费者的场景。
         *        使用原子操作来保证线程安全，避免了锁的开销。
         */
        template <typename T>
        class LockFreeRingBuffer
        {
        private:
            size_t m_capacity;             // 缓冲区容量
            std::vector<T> m_buffer;       // 底层存储
            std::atomic<size_t> m_head{0}; // 头指针（读取位置）
            std::atomic<size_t> m_tail{0}; // 尾指针（写入位置）

        public:
            /// @brief 构造函数。@param p_capacity 缓冲区容量。
            LockFreeRingBuffer(size_t p_capacity = 256)
                : m_capacity(p_capacity), m_buffer(p_capacity) {}
            ~LockFreeRingBuffer() = default;

        public:
            /// @brief 向缓冲区推入一个元素。
            bool push(const T &p_item)
            {
                size_t tail = m_tail.load(std::memory_order_relaxed);
                size_t next_tail = (tail + 1) % m_capacity;
                if (next_tail == m_head.load(std::memory_order_acquire))
                    return false; // 缓冲区已满
                m_buffer[tail] = p_item;
                m_tail.store(next_tail, std::memory_order_release);
                return true;
            }

            /// @brief 从缓冲区弹出一个元素。
            bool pop()
            {
                size_t head = m_head.load(std::memory_order_relaxed);
                size_t next_head = (head + 1) % m_capacity;
                if (head == m_tail.load(std::memory_order_acquire))
                    return false; // 缓冲区为空
                m_head.store(next_head, std::memory_order_release);
                return true;
            }

            /// @brief 查看缓冲区的头部元素，但不弹出。
            std::optional<T> peek()
            {
                size_t head = m_head.load(std::memory_order_relaxed);
                if (head == m_tail.load(std::memory_order_acquire))
                    return std::nullopt; // 缓冲区为空
                T value = m_buffer[head];
                return value;
            }
        };
        // lock_free_ring_buffer.hpp

        // json_element.hpp
        /**
         * @class Element
         * @brief 所有 JSON 元素类型（Value, Array, Object）的抽象基类。
         *        定义了所有 JSON 元素的通用接口，以支持多态。
         */
        class Element
        {
        public:
            /// @brief 虚析构函数。
            virtual ~Element() {}

            /// @brief 判断当前元素是否为简单值 (Value)。
            virtual bool is_value() const noexcept { return false; }
            /// @brief 判断当前元素是否为数组 (Array)。
            virtual bool is_array() const noexcept { return false; }
            /// @brief 判断当前元素是否为对象 (Object)。
            virtual bool is_object() const noexcept { return false; }

            /// @brief 将当前元素转换为 Value 类型指针，若类型不匹配则抛出异常。
            virtual Value *as_value() { throw TypeException("Invalid base type!"); }
            /// @brief 将当前元素转换为 Array 类型指针，若类型不匹配则抛出异常。
            virtual Array *as_array() { throw TypeException("Invalid base type!"); }
            /// @brief 将当前元素转换为 Object 类型指针，若类型不匹配则抛出异常。
            virtual Object *as_object() { throw TypeException("Invalid base type!"); }

            /// @brief 清空元素内容，对于复合类型会递归删除子元素。
            virtual void clear() {}
            /// @brief 创建并返回当前元素的一个深拷贝。
            virtual Element *copy() const = 0;
            /// @brief 将当前元素序列化为紧凑的 JSON 字符串。
            virtual string_t serialize() const noexcept { return ""; }
            /// @brief 将当前元素序列化为带缩进的美化 JSON 字符串。
            virtual string_t pretty_serialize(size_t = 0, char = '\t') const noexcept { return ""; }

            /// @brief 比较两个元素是否相等。
            virtual bool operator==(const Element &other) const noexcept { return false; }
            /// @brief 比较两个元素是否不相等。
            virtual bool operator!=(const Element &other) const noexcept { return true; }
        };
        // json_element.hpp

        // json_value.hpp
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
            explicit Value() : m_value(nullptr) {}
            /// @brief 构造函数，创建指定类型的值。
            explicit Value(std::nullptr_t) : m_value(nullptr) {}
            explicit Value(bool p_value) : m_value(p_value) {}
            explicit Value(int p_value) : m_value(p_value) {}
            explicit Value(float p_value) : m_value(p_value) {}

            explicit Value(const char *p_value) : m_value(string_t(p_value)) {}
            explicit Value(const string_t &p_value) : m_value(p_value) {}
            explicit Value(string_t &&p_value) : m_value(std::move(p_value)) {}
            explicit Value(std::string_view p_value) : m_value(p_value) {}

            /// @brief 拷贝构造函数。
            explicit Value(const Value &other) : m_value(other.m_value) {}
            explicit Value(const Value *other) : m_value(other->m_value) {}
            /// @brief 移动构造函数。
            Value(Value &&other) noexcept : m_value(std::move(other.m_value)) { other.m_value = nullptr; }
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
        // json_value.hpp

        // json_array.hpp
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
            void append(const char *p_value) { append_raw_ptr(new Value(string_t(p_value))); }
            void append(const string_t &p_value) { append_raw_ptr(new Value(p_value)); }

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
        // json_array.hpp

        // json_object.hpp
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
        // json_object.hpp

        // json_ref.hpp
        /**
         * @class Ref
         * @brief 一个 Element 指针的包装类，提供了类似智能指针的功能和便捷的链式访问语法。
         *        例如，可以使用 `json_ref["key"][0]` 的方式来访问嵌套的 JSON 数据。
         */
        class Ref
        {
        private:
            Element *m_ptr; // 指向实际的 Element 对象

        public:
            /// @brief 构造函数，可以接受一个 Element 指针。
            Ref(Element *p_ptr = nullptr) : m_ptr(p_ptr) {}

            /// @brief 重载 [] 运算符，用于访问 Object 的成员。
            Ref operator[](string_v_t p_key)
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
        Ref make_object(std::initializer_list<std::pair<string_t, Ref>> p_list)
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
        Ref make_array(std::initializer_list<Ref> p_list)
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
        Ref make_value(std::nullptr_t) { return Ref(new Value(nullptr)); }
        Ref make_value(bool p_val) { return Ref(new Value(p_val)); }
        Ref make_value(int p_val) { return Ref(new Value(p_val)); }
        Ref make_value(float p_val) { return Ref(new Value(p_val)); }
        Ref make_value(const string_t p_val) { return Ref(new Value(p_val)); }
        Ref make_value(char *p_val) { return Ref(new Value((string_t)p_val)); }
        // json_ref.hpp

        // json_tokenizer.hpp
        // 定义 JSON 解析过程中的各种词法单元（Token）类型
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

        // 定义 Token 结构体，用于存储词法单元的类型和对应的值
        struct Token
        {
            TokenType type;
            std::string_view value;
        };

        /**
         * @class Tokenizer
         * @brief 词法分析器，负责将输入的 JSON 字符串分解成一系列的 Token。
         */
        class Tokenizer
        {
        private:
            std::string m_str; // 要解析的原始字符串
            size_t m_pos;      // 当前解析位置

            size_t line;   // 当前行号
            size_t column; // 当前列号

            Token m_current_token; // 当前已解析出的 Token

        public:
            /// @brief 构造函数。@param p_str 要解析的 JSON 字符串。
            Tokenizer(const std::string &p_str)
                : m_str(p_str), m_pos(0),
                  line(1), column(1) { consume(); } // 初始化时即读取第一个 token
            Tokenizer(const Tokenizer &other) = default;
            ~Tokenizer() = default;

            /// @brief 查看当前的 Token，但不移动解析位置。
            const Token peek() const noexcept { return m_current_token; }
            /// @brief 消费当前的 Token，并读取下一个 Token。
            void consume() { m_current_token = read_next_token(); }

        private:
            /// @brief 检查是否已到达字符串末尾。
            bool eof() const noexcept { return m_pos >= m_str.size(); }
            /// @brief 查看当前位置的字符，但不移动位置。
            char peek_char() const { return m_str[m_pos]; }
            /// @brief 获取当前位置的字符，并移动位置。
            char get_char() { return column++, m_str[m_pos++]; }

            /// @brief 读取并返回下一个有效的 Token。
            Token read_next_token()
            {
                // 1. 跳过所有空白字符
                skip_white_space();
                if (eof())
                    return {TokenType::End, ""}; // 如果已到末尾，返回 End Token

                const size_t start_pos = m_pos;
                char current_ch = peek_char();

                // 2. 根据当前字符判断 Token 类型
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
                    return parse_string(); // 解析字符串
                case 't':
                case 'f':
                    return parse_bool(); // 解析布尔值
                case 'n':
                    return parse_null(); // 解析 null
                default:
                    if (isdigit(current_ch) || current_ch == '-')
                        return parse_number(); // 解析数字
                    else
                        throw ParseException(line, column, (std::string) "Unexpected character '" + std::string(1, current_ch) + "'");
                    break;
                }
            }

        private:
            /// @brief 跳过空白字符（空格、制表符、换行符）。
            inline void skip_white_space()
            {
                while (!eof() && (peek_char() == '\n' || peek_char() == '\t' || peek_char() == ' '))
                {
                    if (get_char() == '\n') // 如果是换行符，更新行号和列号
                        line++, column = 1;
                }
            }

            /// @brief 解析数字 Token (整数或浮点数)。
            Token parse_number()
            {
                const size_t start_pos = m_pos;
                bool is_float = false;

                // 1. 处理可选的负号
                if (!eof() && peek_char() == '-')
                    get_char();

                // 2. 读取整数部分
                while (!eof() && isdigit(peek_char()))
                    get_char();

                // 3. 如果有小数点，则标记为浮点数并读取小数部分
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

            /// @brief 解析布尔值 Token (true 或 false)。
            Token parse_bool()
            {
                using namespace std::literals::string_view_literals;

                const size_t start_pos = m_pos;
                // 1. 尝试匹配 "true"
                if (m_str.size() - start_pos >= 4 && std::string_view(m_str.data() + start_pos, 4) == "true"sv)
                {
                    m_pos += 4;
                    column += 4;
                    return {TokenType::Bool, std::string_view(m_str.data() + start_pos, 4)};
                }
                // 2. 尝试匹配 "false"
                if (m_str.size() - start_pos >= 5 && std::string_view(m_str.data() + start_pos, 5) == "false"sv)
                {
                    m_pos += 5;
                    column += 5;
                    return {TokenType::Bool, std::string_view(m_str.data() + start_pos, 5)};
                }
                // 3. 匹配失败，抛出异常
                throw ParseException(line, column, "Invalid boolean literal");
            }

            /// @brief 解析字符串 Token。
            Token parse_string()
            {
                // 1. 消费开头的引号 "
                get_char();
                const size_t start_pos_content = m_pos;
                while (!eof())
                {
                    char ch = peek_char();
                    // 2. 处理转义字符
                    if (ch == '\\')
                    {
                        get_char(); // 消费 '\'
                        if (!eof())
                            get_char(); // 消费被转义的字符
                    }
                    // 3. 遇到结尾的引号，字符串结束
                    else if (ch == '"')
                    {
                        size_t end_pos_content = m_pos;
                        get_char(); // 消费结尾的引号 "
                        return {TokenType::String, std::string_view(m_str.data() + start_pos_content, end_pos_content - start_pos_content)};
                    }
                    // 4. 处理普通字符
                    else
                        get_char();
                }
                // 5. 如果直到字符串末尾也没找到闭合引号，则抛出异常
                throw ParseException(line, column, "Unterminated string literal");
            }

            /// @brief 解析 null Token。
            Token parse_null()
            {
                using namespace std::literals::string_view_literals;

                const size_t start_pos = m_pos;
                // 1. 尝试匹配 "null"
                if (m_str.size() - start_pos >= 4 && std::string_view(m_str.data() + start_pos, 4) == "null"sv)
                {
                    m_pos += 4;
                    column += 4;
                    return {TokenType::Null, std::string_view(m_str.data() + start_pos, 4)};
                }
                // 2. 匹配失败，抛出异常
                throw ParseException(line, column, "Invalid null literal");
            }
        };
        // json_tokenizer.hpp

        // json_parser.hpp
        /**
         * @class Parser
         * @brief 语法分析器，采用递归下降的方式将 Token 流解析成一棵由 Element 构成的 JSON 树。
         */
        class Parser
        {
        private:
            // LockFreeRingBuffer<Token> m_buffer;
            Tokenizer m_tokenizer; // 内嵌一个词法分析器

        public:
            /// @brief 构造函数。@param p_str 要解析的 JSON 字符串。
            Parser(const std::string &p_str /*, size_t capacity = 16384*/)
                : m_tokenizer(p_str) /*, m_buffer(capacity)*/ {}
            /// @brief 构造函数。@param p_tokenizer 外部传入的词法分析器。
            Parser(Tokenizer &p_tokenizer /*, size_t capacity*/)
                : m_tokenizer(p_tokenizer) /*, m_buffer(capacity)*/ {}

            /// @brief 解析的入口函数，开始整个解析过程。
            Ref parse() { return Ref(parse_value()); }

            // 多线程版本，但是性能不如单线程改回去了
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
            /// @brief 查看下一个 Token。
            Token peek() { return m_tokenizer.peek(); }
            /// @brief 消费当前 Token。
            void consume() { m_tokenizer.consume(); }
            // 多线程版本的跨线程交互数据
            // Token peek()
            // {
            //     std::optional<Token> token;
            //     while (!(token = m_buffer.peek()).has_value())
            //     {
            //         std::this_thread::sleep_for(std::chrono::microseconds(10));
            //         // std::cout << "empty!" << std::endl;
            //     }
            //     return token.value();
            // }
            // void consume()
            // {
            //     while (!m_buffer.pop())
            //     {
            //         std::this_thread::sleep_for(std::chrono::microseconds(10));
            //         // std::cout << "empty!" << std::endl;
            //     }
            // }

            /// @brief 解析一个通用的 JSON 值（可能是 object, array, string, number, bool, null）。
            /// 这是递归下降的核心分发函数。
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
                    // 使用 C++17 的 from_chars 高效转换
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
                    try
                    {
                        // stof 需要一个有所有权的 string，所以创建一个临时对象
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

            /// @brief 解析一个 JSON 对象。
            Object *parse_object()
            {
                // 1. 消费 '{'
                consume();

                Object *obj = new Object();

                // 2. 处理空对象 {} 的情况
                if (peek().type == TokenType::ObjectEnd)
                {
                    consume();
                    return obj;
                }

                // 3. 循环解析键值对
                while (true)
                {
                    // 3.1 解析键（必须是字符串）
                    Token key_token = peek();
                    if (key_token.type != TokenType::String)
                        throw Exception("Expected string key in object!");
                    auto key = key_token.value;
                    consume();

                    // 3.2 消费 ':'
                    if (peek().type != TokenType::Colon)
                        throw Exception("Expected colon after key!");
                    consume();

                    // 3.3 递归解析值，并插入到对象中
                    obj->insert_raw_ptr(key, parse_value());

                    // 3.4 查看下一个 token 是 '}' 还是 ','
                    Token next_token = peek();
                    if (next_token.type == TokenType::ObjectEnd)
                    {
                        consume(); // 消费 '}'
                        break;     // 对象解析结束
                    }
                    else if (next_token.type == TokenType::Comma)
                        consume(); // 消费 ','，继续循环
                    else
                        throw Exception("Expected ',' or '}' in object");
                }

                return obj;
            }

            /// @brief 解析一个 JSON 数组。
            Array *parse_array()
            {
                // 1. 消费 '['
                consume();
                Array *arr = new Array();

                // 2. 处理空数组 [] 的情况
                if (peek().type == TokenType::ArrayEnd)
                {
                    consume();
                    return arr;
                }

                // 3. 循环解析数组成员
                while (true)
                {
                    // 3.1 递归解析数组成员的值
                    arr->append_raw_ptr(parse_value());

                    // 3.2 查看下一个 token 是 ']' 还是 ','
                    Token next_token = peek();
                    if (next_token.type == TokenType::ArrayEnd)
                    {
                        consume(); // 消费 ']'
                        break;     // 数组解析结束
                    }
                    else if (next_token.type == TokenType::Comma)
                        consume(); // 消费 ','，继续循环
                    else
                        throw Exception("Expected ',' or ']' in array");
                }

                return arr;
            }
        };
        // json_parser.hpp
    }
}

#endif // INCLUDE_JSON