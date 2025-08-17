#ifndef INCLUDE_JSON_CHANNEL
#define INCLUDE_JSON_CHANNEL

#include <cstdint>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>

namespace pjh_std
{
    namespace json
    {
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
                m_cond_not_full.notify_one();
                return item;
            }
        };
    }
}

#endif // INCLUDE_JSON_CHANNEL