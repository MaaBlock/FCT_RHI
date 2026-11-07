
#include "../ThirdParty.h"
#include "../Context/VK_Context.h"
#include "./VK_DescriptorPool.h"
namespace FCT
{
    namespace RHI
    {
        VK_DescriptorPool::VK_DescriptorPool(VK_Context* ctx)
        {
            m_ctx = ctx;
        }

        void VK_DescriptorPool::create()
        {
            const uint32_t frames = std::max(1u, m_ctx->maxFrameInFlight());
            std::vector<vk::DescriptorPoolSize> poolSizes = {
                {vk::DescriptorType::eUniformBuffer, frames * 32768},
                {vk::DescriptorType::eCombinedImageSampler, frames * 32768},
                {vk::DescriptorType::eStorageBuffer, frames * 8192},
                {vk::DescriptorType::eStorageImage, frames * 8192},
                {vk::DescriptorType::eSampledImage, frames * 32768},
                {vk::DescriptorType::eSampler, frames * 16384}
            };

            vk::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.maxSets = frames * 65536;
            poolInfo.setPoolSizes(poolSizes);

            poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

            m_descriptorPool = m_ctx->device().createDescriptorPool(poolInfo);
        }
    }
}