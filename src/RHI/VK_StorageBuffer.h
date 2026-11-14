/**
 * @file VK_StorageBuffer.h
 * @brief Vulkan 后端 StorageBuffer 真实硬件驱动实现
 */
#pragma once

#include "./StorageBuffer.h"
#include <vulkan/vulkan.hpp>

namespace FCT
{
    class VK_Context;

    namespace RHI
    {
        /**
         * @brief Vulkan 存储缓冲区 (SSBO) 真实硬件驱动封装
         */
        class VK_StorageBuffer : public StorageBuffer
        {
        public:
            VK_StorageBuffer(VK_Context* ctx);
            VK_StorageBuffer(VK_Context* ctx, size_t size);
            ~VK_StorageBuffer() override;

            Status create(size_t size) override;
            size_t size() const override { return m_size; }
            void* map() override;
            void unmap() override;
            void updateData(const void* data, size_t size, size_t offset = 0) override;
            uint64_t getDeviceAddress() const override { return m_deviceAddress; }

            vk::Buffer getBuffer() const noexcept { return m_buffer; }
            vk::DeviceMemory getMemory() const noexcept { return m_memory; }
            const vk::DescriptorBufferInfo& getDescriptorInfo() const noexcept { return m_descriptorInfo; }
            const vk::DescriptorBufferInfo& currentBufferInfo() const noexcept { return m_descriptorInfo; }

        private:
            void cleanup();

            VK_Context* m_ctx = nullptr;
            vk::Buffer m_buffer = nullptr;
            vk::DeviceMemory m_memory = nullptr;
            void* m_mappedPtr = nullptr;
            size_t m_size = 0;
            uint64_t m_deviceAddress = 0;
            vk::DescriptorBufferInfo m_descriptorInfo{};
        };
    }
}
