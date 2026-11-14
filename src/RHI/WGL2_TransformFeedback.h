#pragma once

#include "TransformFeedback.h"
#include "../Context/WGL2_ContextCommon.h"
#include <array>

namespace FCT {
    class WGL2_Context;

namespace RHI {

    class WGL2_TransformFeedback : public TransformFeedback {
    public:
        explicit WGL2_TransformFeedback(WGL2_Context* ctx);
        ~WGL2_TransformFeedback() override;

        Status create() override;
        void destroy() override;

        void bind() override;
        void unbind() override;

        Status bindBufferBase(uint32_t slot, uint32_t bufferHandle) override;
        Status bindBufferRange(uint32_t slot, uint32_t bufferHandle, size_t offset, size_t size) override;

        Status begin(FeedbackPrimitiveMode mode) override;
        Status end() override;
        Status pause() override;
        Status resume() override;

        bool isActive() const noexcept override { return m_isActive; }
        bool isPaused() const noexcept override { return m_isPaused; }
        FeedbackPrimitiveMode getCurrentMode() const noexcept override { return m_currentMode; }
        uint32_t getNativeHandle() const noexcept override { return m_xfbHandle; }

    private:
        WGL2_Context* m_ctx{nullptr};
        GLuint m_xfbHandle{0};
        bool m_isActive{false};
        bool m_isPaused{false};
        FeedbackPrimitiveMode m_currentMode{FeedbackPrimitiveMode::Points};
        std::array<GLuint, 4> m_boundBuffers{0, 0, 0, 0};
    };

} // namespace RHI
} // namespace FCT
