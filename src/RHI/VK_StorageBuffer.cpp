/**
 * @file VK_StorageBuffer.cpp
 * @brief Vulkan 后端 StorageBuffer 显存分配、映射与设备地址获取落地实现
 */
#include "../ThirdParty.h"
#include "../Context/VK_Context.h"
#include "./VK_StorageBuffer.h"
#include <cassert>
#include <cstring>

namespace FCT
{
    namespace RHI
    {
        VK_StorageBuffer::VK_StorageBuffer(VK_Context* ctx)
            : m_ctx(ctx)
        {
            assert(m_ctx && "VK_StorageBuffer: VK_Context 不可为空");
        }

        VK_StorageBuffer::VK_StorageBuffer(VK_Context* ctx, size_t size)
            : m_ctx(ctx)
        {
            assert(m_ctx && "VK_StorageBuffer: VK_Context 不可为空");
            if (size > 0) {
                Status status = create(size);
                assert(status.ok() && "VK_StorageBuffer: 自动创建显存缓冲失败");
            }
        }

        VK_StorageBuffer::~VK_StorageBuffer()
        {
            cleanup();
        }

        void VK_StorageBuffer::cleanup()
        {
            if (m_mappedPtr && m_ctx && m_memory) {
                m_ctx->device().unmapMemory(m_memory);
                m_mappedPtr = nullptr;
            }
            if (m_buffer && m_ctx) {
                m_ctx->device().destroyBuffer(m_buffer);
                m_buffer = nullptr;
            }
            if (m_memory && m_ctx) {
                m_ctx->device().freeMemory(m_memory);
                m_memory = nullptr;
            }
            m_size = 0;
            m_deviceAddress = 0;
            m_descriptorInfo = vk::DescriptorBufferInfo{};
        }

        Status VK_StorageBuffer::create(size_t size)
        {
            if (size == 0) {
                return InvalidArgumentError("VK_StorageBuffer::create: 缓冲尺寸不能为 0");
            }

            cleanup();
            m_size = size;

            vk::Device device = m_ctx->device();

            vk::BufferCreateInfo bufferInfo{};
            bufferInfo.size = m_size;
            bufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer |
                               vk::BufferUsageFlagBits::eTransferDst |
                               vk::BufferUsageFlagBits::eTransferSrc;

            if (m_ctx->isBufferDeviceAddressSupported()) {
                bufferInfo.usage |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
                bufferInfo.usage |= vk::BufferUsageFlagBits::eShaderBindingTableKHR;
            }
            bufferInfo.sharingMode = vk::SharingMode::eExclusive;

            m_buffer = device.createBuffer(bufferInfo);
            assert(m_buffer && "VK_StorageBuffer::create: 创建 vk::Buffer 失败");

            vk::MemoryRequirements memReq = device.getBufferMemoryRequirements(m_buffer);

            vk::MemoryAllocateInfo allocInfo{};
            allocInfo.allocationSize = memReq.size;
            allocInfo.memoryTypeIndex = m_ctx->findMemoryType(
                memReq.memoryTypeBits,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );

            // 严格遵循 VUID-03333 规范：设置 eShaderDeviceAddress 时必须挂载 eDeviceAddress 分配标志
            vk::MemoryAllocateFlagsInfo flagsInfo{};
            if (bufferInfo.usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
                flagsInfo.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;
                allocInfo.pNext = &flagsInfo;
            }

            m_memory = device.allocateMemory(allocInfo);
            assert(m_memory && "VK_StorageBuffer::create: 分配 vk::DeviceMemory 失败");

            device.bindBufferMemory(m_buffer, m_memory, 0);

            // 保持持久映射提升频繁更新性能
            m_mappedPtr = device.mapMemory(m_memory, 0, m_size);
            assert(m_mappedPtr && "VK_StorageBuffer::create: 映射显存失败");

            // 查询 64 位 GPU 物理设备虚拟地址
            if (bufferInfo.usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
                vk::BufferDeviceAddressInfo addrInfo{};
                addrInfo.buffer = m_buffer;
                m_deviceAddress = device.getBufferAddress(addrInfo);
            } else {
                m_deviceAddress = 0;
            }

            // 初始化描述符更新元数据
            m_descriptorInfo.buffer = m_buffer;
            m_descriptorInfo.offset = 0;
            m_descriptorInfo.range = m_size;

            return OkStatus();
        }

        void* VK_StorageBuffer::map()
        {
            if (m_mappedPtr) {
                return m_mappedPtr;
            }
            if (m_memory && m_ctx) {
                m_mappedPtr = m_ctx->device().mapMemory(m_memory, 0, m_size);
            }
            return m_mappedPtr;
        }

        void VK_StorageBuffer::unmap()
        {
            if (m_mappedPtr && m_ctx && m_memory) {
                m_ctx->device().unmapMemory(m_memory);
                m_mappedPtr = nullptr;
            }
        }

        void VK_StorageBuffer::updateData(const void* data, size_t size, size_t offset)
        {
            if (!data || size == 0) {
                return;
            }
            assert(offset + size <= m_size && "VK_StorageBuffer::updateData 越界");

            if (m_mappedPtr) {
                memcpy(static_cast<char*>(m_mappedPtr) + offset, data, size);
            } else {
                void* ptr = map();
                if (ptr) {
                    memcpy(static_cast<char*>(ptr) + offset, data, size);
                }
            }
        }
    }
}
