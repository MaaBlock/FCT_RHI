
#pragma once

#include <type_traits>
#include <utility>
#include "../DebugTools/MemoryCheak.h"
#include "RefCount.h"

namespace FCT {

    /**
     * @cond CHINESE
     * @brief 一个简单的唯一所有权智能指针。
     * @tparam T 被管理的对象的类型。
     * @note
     * - 这个智能指针确保当它本身被销毁时，它所管理的对象也会被销毁。
     * - 它通过禁用拷贝构造和拷贝赋值来保证所有权的唯一性。
     * - 所有权可以通过移动构造和移动赋值来转移。
     * - 使用 FCT_DELETE 来销毁对象。
     * - T 类型不能继承自 FCT::RefCount，对于引用计数的对象，请使用 Ptr。
     * @endcond
     */
    template<typename T>
    class UniquePtr {
    public:
        template<typename U> friend class UniquePtr;

        // 编译时检查，确保 T 没有继承自 RefCount
        static_assert(!std::is_base_of<FCT::RefCount, T>::value && !FCT::IsNewRefCountBase<T>::value,
            "UniquePtr cannot manage types derived from FCT::RefCount. Use a reference-counting pointer instead.");

        /**
         * @brief 默认构造函数，创建一个空的 UniquePtr。
         */
        UniquePtr() noexcept : m_ptr(nullptr) {}

        /**
         * @brief 从裸指针构造 UniquePtr，获得其所有权。
         * @param ptr 指向要管理对象的指针。
         */
        explicit UniquePtr(T* ptr) noexcept : m_ptr(ptr) {}

        /**
         * @brief 移动构造函数，从另一个 UniquePtr 转移所有权。
         * @param other 用于转移所有权的另一个 UniquePtr。
         */
        UniquePtr(UniquePtr&& other) noexcept : m_ptr(other.release()) {}

        /**
         * @brief 允许从派生类的 UniquePtr 移动构造。
         */
        template<typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
        UniquePtr(UniquePtr<U>&& other) noexcept : m_ptr(other.release()) {}

        /**
         * @brief 移动赋值运算符，从另一个 UniquePtr 转移所有权。
         * @param other 用于转移所有权的另一个 UniquePtr。
         * @return *this 的引用。
         */
        UniquePtr& operator=(UniquePtr&& other) noexcept {
            if (this != &other) {
                reset(other.release());
            }
            return *this;
        }

        /**
         * @brief 允许从派生类的 UniquePtr 移动赋值。
         */
        template<typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
        UniquePtr& operator=(UniquePtr<U>&& other) noexcept {
            reset(other.release());
            return *this;
        }

        // 禁止拷贝构造和拷贝赋值
        UniquePtr(const UniquePtr&) = delete;
        UniquePtr& operator=(const UniquePtr&) = delete;

        /**
         * @brief 析构函数，销毁所管理的对象。
         */
        ~UniquePtr() {
            reset();
        }

        /**
         * @brief 释放对当前对象的所有权，并返回裸指针。
         * @return 返回之前管理的对象的裸指针，调用者负责释放它。
         */
        T* release() noexcept {
            T* temp = m_ptr;
            m_ptr = nullptr;
            return temp;
        }

        /**
         * @brief 替换管理的对象。
         * @param ptr 指向新对象的指针，默认为 nullptr。
         */
        void reset(T* ptr = nullptr) noexcept {
            if (m_ptr) {
                FCT_DELETE(m_ptr);
            }
            m_ptr = ptr;
        }

        /**
         * @brief 获取裸指针。
         * @return 返回管理的对象的裸指针。
         */
        T* get() const noexcept {
            return m_ptr;
        }

        /**
         * @brief 解引用操作符。
         * @return 返回管理对象的引用。
         */
        T& operator*() const {
            return *m_ptr;
        }

        /**
         * @brief 箭头操作符。
         * @return 返回管理对象的指针。
         */
        T* operator->() const noexcept {
            return m_ptr;
        }

        /**
         * @brief 检查是否管理着一个对象。
         * @return 如果指针非空则返回 true，否则返回 false。
         */
        explicit operator bool() const noexcept {
            return m_ptr != nullptr;
        }

    private:
        T* m_ptr;
    };

    /**
     * @cond CHINESE
     * @brief 创建一个 UniquePtr 的辅助函数，类似于 std::make_unique。
     * @tparam T 要创建的对象的类型。
     * @tparam Args 构造函数参数的类型。
     * @param args 传递给 T 的构造函数的参数。
     * @return 返回一个管理着新创建对象的 UniquePtr。
     * @endcond
     */
    template<typename T, typename... Args>
    UniquePtr<T> makeUnique(Args&&... args) {
        return UniquePtr<T>(FCT_NEW(T, std::forward<Args>(args)...));
    }

} // namespace FCT
