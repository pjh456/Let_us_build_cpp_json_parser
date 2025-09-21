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
    }
}

#endif // INCLUDE_JSON_CHANNEL