#include "../ThirdParty.h"
#include "../Context/VK_Context.h"
#include "./VK_DescriptorPool.h"
#include <algorithm>
#include <cassert>

namespace FCT
{
    namespace RHI
    {
        VK_DescriptorPool::VK_DescriptorPool(VK_Context* ctx)
            : m_ctx(ctx)
        {
        }

        VK_DescriptorPool::~VK_DescriptorPool()
        {
            if (m_descriptorPool && m_ctx) {
                m_ctx->device().destroyDescriptorPool(m_descriptorPool);
                m_descriptorPool = nullptr;
            }
        }

        void VK_DescriptorPool::create()
        {
            const uint32_t frames = std::max(1u, m_ctx->maxFrameInFlight());
            std::vector<vk::DescriptorPoolSize> poolSizes = {
                {vk::DescriptorType::eUniformBuffer, frames * 32768},
                {vk::DescriptorType::eCombinedImageSampler, frames * 32768},
                {vk::DescriptorType::eStorageBuffer, frames * 32768},
                {vk::DescriptorType::eStorageImage, frames * 16384},
                {vk::DescriptorType::eSampledImage, frames * 65536},
                {vk::DescriptorType::eSampler, frames * 16384}
            };

            vk::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.maxSets = frames * 65536;
            poolInfo.setPoolSizes(poolSizes);

            // 关键：必须显式声明 eUpdateAfterBind 标志位，否则无法分配 Bindless 描述符集
            poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
                           | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;

            m_descriptorPool = m_ctx->device().createDescriptorPool(poolInfo);
            assert(m_descriptorPool && "VK_DescriptorPool::create: 无法创建 Vulkan 描述符池");
        }
    }
}