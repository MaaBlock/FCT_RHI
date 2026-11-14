//
// Created by Administrator on 2025/3/27.
//
#ifndef FCT_VK_BUFFER_H
#define FCT_VK_BUFFER_H
#include "../ThirdParty.h"
#include "./Image.h"
namespace FCT
{
    class VK_Context;
    namespace RHI
    {
        class VK_Image : public Image {
        public:
            VK_Image(VK_Context* ctx);
            ~VK_Image();
            void create() override;
            void create(bool uploadData) override;
            void uploadInitialData();
            void updateData(const void* data, size_t dataSize) override;
            void updateData(const void* data, size_t dataSize, Fence* fence, std::function<void()>* onCompletion) override;
            void updateSubRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t arrayLayer, uint32_t mipLevel, const void* pixelData, size_t rowPitch = 0) override;
            void uploadAsync(std::vector<uint8_t> data, std::function<void()> callback) override;
            
            void create(vk::Image image); // 注意，格式信息还是给自己填
            vk::Image getVkImage();
            vk::Image image() const { return m_image; }
        private:
            VK_Context* m_ctx;
            vk::Image m_image;
            vk::DeviceMemory m_memory;
            bool m_owns;
        };
    }
}


#endif //FCT_VK_BUFFER_H
