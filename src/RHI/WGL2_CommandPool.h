#pragma once

#include "./CommandPool.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_CommandPool : public CommandPool {
public:
    explicit WGL2_CommandPool(WGL2_Context* ctx);
    ~WGL2_CommandPool() override = default;

    void create() override;
    CommandBuffer* createCommandBuffer() override;

private:
    WGL2_Context* m_ctx{nullptr};
};

} // namespace RHI
} // namespace FCT
