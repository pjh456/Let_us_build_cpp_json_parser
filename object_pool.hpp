#ifndef INCLUDE_JSON_OBJECT_POOL
#define INCLUDE_JSON_OBJECT_POOL

#include <stdexcept>
#include <utility>

namespace pjh_std
{
    namespace json
    {
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
    }
}

#endif // INCLUDE_JSON_OBJECT_POOL