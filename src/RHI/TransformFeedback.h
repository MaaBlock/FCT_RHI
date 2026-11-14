#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include "../Base/Status.h"
#include "../Type/type.h"

namespace FCT::RHI {

    enum class FeedbackPrimitiveMode : uint8_t {
        Points = 0,
        Lines = 1,
        Triangles = 2
    };

    enum class FeedbackBufferMode : uint8_t {
        Interleaved = 0,
        Separate = 1
    };

    /**
     * @brief WebGL2 / GLES 3.0 TransformFeedback 硬件抽象基类 (FEAT-39)
     * Note: 负责 GPU 顶点着色器变换产物的无光栅化捕获流，支持多缓冲槽位绑定与暂停/恢复状态机。
     */
    class TransformFeedback {
    public:
        virtual ~TransformFeedback() = default;

        virtual Status create() = 0;
        virtual void destroy() = 0;

        virtual void bind() = 0;
        virtual void unbind() = 0;

        virtual Status bindBufferBase(uint32_t slot, uint32_t bufferHandle) = 0;
        virtual Status bindBufferRange(uint32_t slot, uint32_t bufferHandle, size_t offset, size_t size) = 0;

        virtual Status begin(FeedbackPrimitiveMode mode) = 0;
        virtual Status end() = 0;
        virtual Status pause() = 0;
        virtual Status resume() = 0;

        virtual bool isActive() const noexcept = 0;
        virtual bool isPaused() const noexcept = 0;
        virtual FeedbackPrimitiveMode getCurrentMode() const noexcept = 0;
        virtual uint32_t getNativeHandle() const noexcept = 0;
    };

} // namespace FCT::RHI
