//
// Created by Administrator on 2025/4/14.
//
#include "./ConstBuffer.h"
#ifndef VK_CONSTBUFFER_H
#define VK_CONSTBUFFER_H
namespace FCT {
    namespace RHI
    {
        class VK_ConstBuffer : public ConstBuffer{
        public:
            VK_ConstBuffer(VK_Context* ctx);
            ~VK_ConstBuffer() override;
            void mapData() override;
            void upload(const void* data, size_t size) override;
            void upload(const void* data, size_t size, size_t offset) override;
            void checkAndUpdateCurrentFrame();
            const vk::DescriptorBufferInfo& currentBufferInfo();

            const vk::DescriptorBufferInfo& currentBufferInfoWithoutUpdata() const;
            void create() override;
        protected:
            VK_Context* m_ctx;
            std::vector<vk::Buffer> m_buffers;
            std::vector<vk::DeviceMemory> m_memories;
            std::vector<void*> m_mappedData;
            std::vector<vk::DescriptorBufferInfo> m_bufferInfos;
            std::vector<bool> m_frameNeedsUpdate;
        };
    }
}


#endif //VK_CONSTBUFFER_H
