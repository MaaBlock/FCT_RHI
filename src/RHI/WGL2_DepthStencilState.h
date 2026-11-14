#pragma once

#include "./DepthStencilState.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_DepthStencilState : public DepthStencilState {
public:
    explicit WGL2_DepthStencilState(WGL2_Context* ctx) : m_ctx(ctx) {}
    ~WGL2_DepthStencilState() override = default;

    void create() override {}

private:
    WGL2_Context* m_ctx{nullptr};
};

} // namespace RHI
} // namespace FCT
