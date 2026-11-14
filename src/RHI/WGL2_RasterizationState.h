#pragma once

#include "./RasterizationState.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_RasterizationState : public RasterizationState {
public:
    explicit WGL2_RasterizationState(WGL2_Context* ctx) : m_ctx(ctx) {}
    ~WGL2_RasterizationState() override = default;

    void create() override {}

private:
    WGL2_Context* m_ctx{nullptr};
};

} // namespace RHI
} // namespace FCT
