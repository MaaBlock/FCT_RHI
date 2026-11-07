//
// Created by Administrator on 2025/3/16.
//

#ifndef FCT_TOOLDEFINE_H
#define FCT_TOOLDEFINE_H

#include <thread>
#include <chrono>
#include <atomic>
#include <cstdint>

#if defined(_MSC_VER) || defined(__i386__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_IX86)
#include <immintrin.h>
#define FCT_CPU_PAUSE() _mm_pause()
#elif defined(__aarch64__) || defined(__arm__)
#define FCT_CPU_PAUSE() __asm__ __volatile__("yield" ::: "memory")
#else
#define FCT_CPU_PAUSE() std::this_thread::yield()
#endif

namespace FCT {

    /// 自适应退避等待策略：
    /// 1. 前 32 次迭代执行轻量 CPU PAUSE 指令（硬件级流水线友好，纳秒级响应，零系统调用）
    /// 2. 32~64 次迭代让出当前 CPU 时间片（std::this_thread::yield）
    /// 3. 超过 64 次后采用微秒级主动休眠，防止 CPU 单核 100% 盲转饱和
    inline void backoffYield(uint32_t& spinCount) noexcept
    {
        if (spinCount < 32)
        {
            FCT_CPU_PAUSE();
        }
        else if (spinCount < 64)
        {
            std::this_thread::yield();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
        ++spinCount;
    }

    /// 条件求值重载：对于 std::atomic<bool> 强制使用 acquire 内存序建立同步
    template <typename T>
    inline bool evaluateCondition(const std::atomic<T>& atomicVar) noexcept
    {
        return atomicVar.load(std::memory_order_acquire);
    }

    /// 普通 bool 表达式求值重载（兼容非原子变量与条件表达式）
    inline bool evaluateCondition(bool boolVal) noexcept
    {
        return boolVal;
    }

    /// 条件重置重载：对于 std::atomic<bool> 使用 release 内存序发布重置
    template <typename T>
    inline void resetCondition(std::atomic<T>& atomicVar) noexcept
    {
        atomicVar.store(false, std::memory_order_release);
    }

    /// 普通 bool 变量重置重载
    inline void resetCondition(bool& boolVar) noexcept
    {
        boolVar = false;
    }

} // namespace FCT

#define FCT_WAIT_FOR(condition) \
    do { \
        uint32_t fct_spin_count = 0; \
        while (!FCT::evaluateCondition(condition)) \
        { \
            FCT::backoffYield(fct_spin_count); \
        } \
        FCT::resetCondition(condition); \
    } while (0)

#define FCT_WAIT_FOR_WITH_TERM(condition, termCondition) \
    do { \
        uint32_t fct_spin_count = 0; \
        while (!FCT::evaluateCondition(condition) && !(termCondition)) \
        { \
            FCT::backoffYield(fct_spin_count); \
        } \
        if (FCT::evaluateCondition(condition)) \
        { \
            FCT::resetCondition(condition); \
        } \
    } while (0)

#endif //FCT_TOOLDEFINE_H

