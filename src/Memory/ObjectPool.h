//
// Created by Administrator on 2025/5/8.
//
#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H
#include "../DebugTools/MemoryCheak.h"
#include "../MutilThreadBase/RefCount.h"

namespace FCT
{
    constexpr size_t SingleChunkObjectNums = 1024;

    template<typename T>
    class ObjectPoolChunk
    {
    public:
        ObjectPoolChunk() : m_objects(AllocatorNew<T>(SingleChunkObjectNums))
        {
            m_size = 0;
        }
        ObjectPoolChunk(const ObjectPoolChunk& other) = delete;
        ObjectPoolChunk(ObjectPoolChunk&& other) noexcept
            : m_size(other.m_size), m_objects(other.m_objects)
        {
            other.m_objects = nullptr;
            other.m_size = 0;
        }
        ~ObjectPoolChunk()
        {
            if (m_objects)
            {
                clear();
                AllocatorDelete(m_objects);
            }
        }
        template<typename... Args>
        T* addSingle(size_t index, Args&&... args)
        {
            m_size = index + 1;
            return New<T>(m_objects + index,
                          std::forward<Args>(args)...);
        }
        T* addSingle(size_t index)
        {
            m_size = index + 1;
            return New<T>(m_objects + index);
        }
        T* get(size_t index) const
        {
            return m_objects + index;
        }
        void clear()
        {
            if (m_objects)
            {
                for (size_t i = 0; i < m_size; ++i)
                {
                    Destroy(m_objects + i);
                }
                m_size = 0;
            }
        }
    protected:
        size_t m_size;
        T* m_objects;
    };

    /**
     * @brief 线程安全分块对象池，使用无上限 LIFO 回收栈替代定长 1024 队列，消除高并发分配竞态与槽位泄漏
     */
    template<typename T, typename... Args>
    class ObjectPool
    {
    public:
        using DestroyCallback = std::function<void(T*)>;

        explicit ObjectPool(Args... args)
            : m_size(0)
            , m_destroyCallback(nullptr)
            , m_constructorArgs(std::forward<Args>(args)...)
        {
        }

        ~ObjectPool() = default;

        T* alloc()
        {
            std::lock_guard<std::mutex> lock(m_poolMutex);
            if (!m_freeIndices.empty())
            {
                const size_t index = m_freeIndices.back();
                m_freeIndices.pop_back();
                return m_chunks[index / SingleChunkObjectNums].get(index % SingleChunkObjectNums);
            }
            return addSingleLocked();
        }

        void setDestroyCallback(DestroyCallback callback)
        {
            std::lock_guard<std::mutex> lock(m_poolMutex);
            m_destroyCallback = std::move(callback);
        }

        size_t totalAllocatedCount() const
        {
            std::lock_guard<std::mutex> lock(m_poolMutex);
            return m_size;
        }

        size_t freeCount() const
        {
            std::lock_guard<std::mutex> lock(m_poolMutex);
            return m_freeIndices.size();
        }

    protected:
        void recycleSlot(T* ptr, size_t index)
        {
            DestroyCallback cb;
            {
                std::lock_guard<std::mutex> lock(m_poolMutex);
                cb = m_destroyCallback;
            }
            if (cb)
            {
                cb(ptr);
            }
            std::lock_guard<std::mutex> lock(m_poolMutex);
            m_freeIndices.push_back(index);
        }

        template<typename U>
        void tryBindPoolDeleter(FCT::DeletableTrait<U>* trait, size_t index)
        {
            trait->setDeleter([this, index](U* basePtr)
            {
                recycleSlot(static_cast<T*>(basePtr), index);
            });
        }

        void tryBindPoolDeleter(void*, size_t) noexcept
        {
        }

        T* addSingleLocked()
        {
            T* ret = nullptr;
            if ((m_size % SingleChunkObjectNums) != 0)
            {
                ret = std::apply([this](auto&... args)
                {
                    return m_chunks[m_size / SingleChunkObjectNums]
                        .addSingle(m_size % SingleChunkObjectNums, args...);
                }, m_constructorArgs);
            }
            else
            {
                m_chunks.emplace_back();
                ret = std::apply([this](auto&... args)
                {
                    return m_chunks.back()
                        .addSingle(m_size % SingleChunkObjectNums, args...);
                }, m_constructorArgs);
            }

            tryBindPoolDeleter(ret, m_size);
            ++m_size;
            return ret;
        }

        mutable std::mutex m_poolMutex;
        size_t m_size;
        std::vector<ObjectPoolChunk<T>> m_chunks;
        std::vector<size_t> m_freeIndices;
        DestroyCallback m_destroyCallback;
        std::tuple<Args...> m_constructorArgs;
    };
}
#endif //OBJECTPOOL_H
