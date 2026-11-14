#pragma once

#include "./Pass.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_PassGroup;

class WGL2_Pass : public Pass {
public:
    explicit WGL2_Pass(WGL2_Context* ctx);
    ~WGL2_Pass() override = default;

    void create(PassGroup* group) override;
    void beginSubmit(CommandBuffer* cmdBuf) override;
    void endSubmit() override;
    void executeClear(CommandBuffer* cmdBuf) override;

private:
    WGL2_Context* m_ctx{nullptr};
    WGL2_PassGroup* m_wgl2Group{nullptr};
};

} // namespace RHI
} // namespace FCT
