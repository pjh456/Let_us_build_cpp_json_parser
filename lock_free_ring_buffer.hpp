#ifndef INCLUDE_JSON_LOCK_FREE_RING_BUFFER
#define INCLUDE_JSON_LOCK_FREE_RING_BUFFER

#include <atomic>
#include <optional>
#include <vector>

namespace pjh_std
{
    namespace json
    {
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
    }
}

#endif // INCLUDE_JSON_LOCK_FREE_RING_BUFFER