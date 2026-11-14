#pragma once

#include "./StorageBuffer.h"
#include "../Context/WGL2_ContextCommon.h"
#include <vector>

namespace FCT {

class WGL2_Context;

namespace RHI {

struct WGL2_StorageBufferSubRegion {
    uint32_t x{0};
    uint32_t y{0};
    uint32_t width{0};
    uint32_t height{0};
    size_t texelOffset{0};
};

/**
 * @brief WebGL 2.0 / GLES 3.0 2D 数据纹理模拟 StorageBuffer (FEAT-25)
 * 基于 1024 宽 GL_RGBA32UI 2D 纹理，单 Texel 16 字节，纯位运算寻址，
 * 任意线性连续切片至多分解为 3 次 glTexSubImage2D 局部更新。
 */
class WGL2_StorageBuffer : public StorageBuffer {
public:
    explicit WGL2_StorageBuffer(WGL2_Context* ctx);
    WGL2_StorageBuffer(WGL2_Context* ctx, size_t size);
    ~WGL2_StorageBuffer() override;

    Status create(size_t size) override;
    size_t size() const override { return m_size; }
    void* map() override;
    void unmap() override;
    void updateData(const void* data, size_t size, size_t offset = 0) override;
    uint64_t getDeviceAddress() const override { return static_cast<uint64_t>(m_textureHandle); }

    GLuint getTextureHandle() const noexcept { return m_textureHandle; }
    uint32_t getWidth() const noexcept { return kWidth; }
    uint32_t getHeight() const noexcept { return m_height; }

    static std::vector<WGL2_StorageBufferSubRegion> calculateSubRegions(size_t offset, size_t size);

    static constexpr uint32_t kWidth = 1024;
    static constexpr uint32_t kTexelSizeBytes = 16;
    static constexpr uint32_t kRowSizeBytes = kWidth * kTexelSizeBytes; // 16384 字节

private:
    void cleanup();

    WGL2_Context* m_ctx{nullptr};
    GLuint m_textureHandle{0};
    size_t m_size{0};
    uint32_t m_height{0};
    std::vector<uint8_t> m_cpuShadowBuffer;
};

} // namespace RHI
} // namespace FCT
