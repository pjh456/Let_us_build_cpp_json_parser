#ifndef INCLUDE_JSON_LOCK_FREE_RING_BUFFER
#define INCLUDE_JSON_LOCK_FREE_RING_BUFFER

#include <atomic>
#include <optional>

namespace pjh_std
{
    namespace json
    {
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
    }
}

#endif // INCLUDE_JSON_LOCK_FREE_RING_BUFFER