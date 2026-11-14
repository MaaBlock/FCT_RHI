#pragma once

#include "./DescriptorPool.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_DescriptorPool : public DescriptorPool {
public:
    explicit WGL2_DescriptorPool(WGL2_Context* ctx);
    ~WGL2_DescriptorPool() override = default;

    void create() override;

private:
    WGL2_Context* m_ctx{nullptr};
};

} // namespace RHI
} // namespace FCT
