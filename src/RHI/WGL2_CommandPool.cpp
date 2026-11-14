#include "./WGL2_CommandPool.h"
#include "./WGL2_CommandBuffer.h"
#include "../Context/WGL2_Context.h"

namespace FCT::RHI {

WGL2_CommandPool::WGL2_CommandPool(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

void WGL2_CommandPool::create()
{
}

CommandBuffer* WGL2_CommandPool::createCommandBuffer()
{
    return FCT_NEW(WGL2_CommandBuffer, m_ctx);
}

} // namespace FCT::RHI
