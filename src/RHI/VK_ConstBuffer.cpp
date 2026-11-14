#include "../FCTAPI.h"
#include <cassert>

namespace FCT
{
    namespace RHI
    {
        VK_ConstBuffer::VK_ConstBuffer(VK_Context* ctx)
        {
            m_ctx = ctx;
            m_frameNeedsUpdate.clear();
        }

        VK_ConstBuffer::~VK_ConstBuffer()
        {
            for (auto& buffer : m_buffers)
            {
                m_ctx->device().destroyBuffer(buffer);
            }
            for (auto& memory : m_memories)
            {
                m_ctx->device().freeMemory(memory);
            }
        }

        void VK_ConstBuffer::mapData()
        {
            // Note: m_buffer 已解耦移除，纯数据上传由 upload() 接口统一驱动
        }

        void VK_ConstBuffer::upload(const void* data, size_t size)
        {
            upload(data, size, 0);
        }

        void VK_ConstBuffer::upload(const void* data, size_t size, size_t offset)
        {
            if (!data || size == 0) {
                return;
            }
            uint32_t currentFrame = m_ctx ? m_ctx->currentSubmitFrameIndex() : 0;
            if (currentFrame < m_mappedData.size() && m_mappedData[currentFrame]) {
                size_t maxRange = static_cast<size_t>(m_bufferInfos[currentFrame].range);
                if (offset < maxRange) {
                    size_t copySize = std::min(size, maxRange - offset);
                    memcpy(static_cast<char*>(m_mappedData[currentFrame]) + offset, data, copySize);
                }
            }
        }

        void VK_ConstBuffer::checkAndUpdateCurrentFrame()
        {
            // Note: m_buffer 已移除，无需从 CPU UniformBuffer 拷贝
        }

        const vk::DescriptorBufferInfo& VK_ConstBuffer::currentBufferInfo()
        {
            checkAndUpdateCurrentFrame();
            return m_bufferInfos[m_ctx->currentSubmitFrameIndex()];
        }

        const vk::DescriptorBufferInfo& VK_ConstBuffer::currentBufferInfoWithoutUpdata() const
        {
            return m_bufferInfos[m_ctx->currentSubmitFrameIndex()];
        }

        void VK_ConstBuffer::create()
        {
            vk::Device device = m_ctx->getDevice();
            uint32_t maxFramesInFlight = m_ctx->maxFrameInFlight();

            vk::DeviceSize bufferSize = m_layout.getTotalSize();
            if (bufferSize == 0) {
                bufferSize = 256;
            }

            m_buffers.resize(maxFramesInFlight);
            m_memories.resize(maxFramesInFlight);
            m_mappedData.resize(maxFramesInFlight, nullptr);
            m_frameNeedsUpdate.resize(maxFramesInFlight, false);

            for (uint32_t i = 0; i < maxFramesInFlight; i++) {
                vk::BufferCreateInfo bufferInfo;
                bufferInfo.size = bufferSize;
                bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
                bufferInfo.sharingMode = vk::SharingMode::eExclusive;

                m_buffers[i] = device.createBuffer(bufferInfo);
                assert(m_buffers[i] && "VK_ConstBuffer::create: 无法创建 Uniform Buffer");

                vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(m_buffers[i]);

                vk::MemoryAllocateInfo allocInfo;
                allocInfo.allocationSize = memRequirements.size;
                allocInfo.memoryTypeIndex = m_ctx->findMemoryType(
                    memRequirements.memoryTypeBits,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
                );

                m_memories[i] = device.allocateMemory(allocInfo);
                assert(m_memories[i] && "VK_ConstBuffer::create: 无法分配 Uniform Buffer 显存");

                device.bindBufferMemory(m_buffers[i], m_memories[i], 0);

                m_mappedData[i] = device.mapMemory(m_memories[i], 0, bufferSize);
                assert(m_mappedData[i] && "VK_ConstBuffer::create: 无法映射 Uniform Buffer 内存");

                memset(m_mappedData[i], 0, static_cast<size_t>(bufferSize));
            }

            m_bufferInfos.resize(maxFramesInFlight);
            for (uint32_t i = 0; i < maxFramesInFlight; i++) {
                m_bufferInfos[i].buffer = m_buffers[i];
                m_bufferInfos[i].offset = 0;
                m_bufferInfos[i].range = bufferSize;
            }
        }
    }
}