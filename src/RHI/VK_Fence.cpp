#include "./VK_Fence.h"
#include "../ThirdParty.h"
#include "../Context/VK_Context.h"
namespace FCT
{
    namespace RHI
    {
        VK_Fence::VK_Fence(VK_Context* ctx)
        {
            m_ctx = ctx;
        }

        VK_Fence::~VK_Fence()
        {
            VK_Fence::destroy();
        }

        void VK_Fence::reset()
        {
            m_ctx->device().resetFences(1, &m_fence);
        }

        void VK_Fence::create()
        {
            if (count() == 0)
            {
                addRef();
            }
            if (m_fence)
            {
                m_ctx->device().destroyFence(m_fence, nullptr);
                m_fence = nullptr;
            }
            vk::FenceCreateInfo createInfo{};
            createInfo.flags |= m_signaled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlags(0);
            m_fence = m_ctx->device().createFence(createInfo);
        }

        void VK_Fence::waitFor()
        {
            m_ctx->device().waitForFences(1, &m_fence, VK_TRUE, UINT64_MAX);
        }

        void VK_Fence::destroy()
        {
            if (m_fence)
            {
                m_ctx->device().destroyFence(m_fence, nullptr);
                m_fence = nullptr;
            }
        }
    }
}
