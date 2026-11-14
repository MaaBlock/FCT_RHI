#include "./WGL2_Fence.h"
#include "../Context/WGL2_Context.h"

namespace FCT::RHI {

WGL2_Fence::WGL2_Fence(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

WGL2_Fence::~WGL2_Fence()
{
    destroy();
}

void WGL2_Fence::reset()
{
    destroy();
    m_signaled = false;
}

void WGL2_Fence::create()
{
    if (m_sync != nullptr) {
        glDeleteSync(m_sync);
        m_sync = nullptr;
    }
    m_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    m_signaled = false;
}

void WGL2_Fence::waitFor()
{
    if (m_sync != nullptr) {
        glClientWaitSync(m_sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ULL);
        glDeleteSync(m_sync);
        m_sync = nullptr;
    }
    m_signaled = true;
}

void WGL2_Fence::destroy()
{
    if (m_sync != nullptr) {
        glDeleteSync(m_sync);
        m_sync = nullptr;
    }
}

} // namespace FCT::RHI
