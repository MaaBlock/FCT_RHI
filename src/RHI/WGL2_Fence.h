#pragma once

#include "./Fence.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_Fence : public Fence {
public:
    explicit WGL2_Fence(WGL2_Context* ctx);
    ~WGL2_Fence() override;

    void reset() override;
    void create() override;
    void waitFor() override;
    void destroy() override;

private:
    WGL2_Context* m_ctx{nullptr};
    GLsync m_sync{nullptr};
};

} // namespace RHI
} // namespace FCT
