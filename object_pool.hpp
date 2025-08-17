#ifndef INCLUDE_JSON_OBJECT_POOL
#define INCLUDE_JSON_OBJECT_POOL

#include <stdexcept>
#include <utility>

namespace pjh_std
{
    namespace json
    {
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

        template <typename T, size_t BlockSize = 1024>
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
    }
}

#endif // INCLUDE_JSON_OBJECT_POOL