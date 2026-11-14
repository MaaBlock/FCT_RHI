#pragma once

#include "./Semaphore.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_Semaphore : public Semaphore {
public:
    explicit WGL2_Semaphore(WGL2_Context* ctx);
    ~WGL2_Semaphore() override = default;

    void create() override;
    void destroy() override;

private:
    WGL2_Context* m_ctx{nullptr};
};

} // namespace RHI
} // namespace FCT
