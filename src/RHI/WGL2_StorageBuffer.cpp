#include "./WGL2_StorageBuffer.h"
#include "../Context/WGL2_Context.h"
#include <cassert>
#include <cstring>

namespace FCT::RHI {

WGL2_StorageBuffer::WGL2_StorageBuffer(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

WGL2_StorageBuffer::WGL2_StorageBuffer(WGL2_Context* ctx, size_t size)
    : m_ctx(ctx)
{
    create(size);
}

WGL2_StorageBuffer::~WGL2_StorageBuffer()
{
    cleanup();
}

void WGL2_StorageBuffer::cleanup()
{
    if (m_textureHandle != 0 && glDeleteTextures) {
        glDeleteTextures(1, &m_textureHandle);
        m_textureHandle = 0;
    }
    m_size = 0;
    m_height = 0;
    m_cpuShadowBuffer.clear();
}

Status WGL2_StorageBuffer::create(size_t size)
{
    cleanup();

    if (size == 0) {
        return OkStatus();
    }

    // 16 字节对齐
    size_t alignedSize = (size + kTexelSizeBytes - 1) & ~(kTexelSizeBytes - 1);
    size_t totalTexels = alignedSize / kTexelSizeBytes;
    m_height = static_cast<uint32_t>((totalTexels + kWidth - 1) / kWidth);
    if (m_height == 0) m_height = 1;

    m_size = alignedSize;
    m_cpuShadowBuffer.resize(m_height * kRowSizeBytes, 0);

    if (glGenTextures) {
        glGenTextures(1, &m_textureHandle);
        if (m_textureHandle != 0) {
            glBindTexture(GL_TEXTURE_2D, m_textureHandle);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32UI, kWidth, m_height);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    return OkStatus();
}

void* WGL2_StorageBuffer::map()
{
    return m_cpuShadowBuffer.data();
}

void WGL2_StorageBuffer::unmap()
{
    if (!m_cpuShadowBuffer.empty()) {
        updateData(m_cpuShadowBuffer.data(), m_size, 0);
    }
}

std::vector<WGL2_StorageBufferSubRegion> WGL2_StorageBuffer::calculateSubRegions(size_t offset, size_t size)
{
    std::vector<WGL2_StorageBufferSubRegion> regions;
    if (size == 0) return regions;

    assert((offset % kTexelSizeBytes == 0) && "StorageBuffer 偏移必须按 16 字节对齐！");
    assert((size % kTexelSizeBytes == 0) && "StorageBuffer 尺寸必须按 16 字节对齐！");

    size_t startTexel = offset >> 4;
    size_t totalTexels = size >> 4;
    size_t endTexel = startTexel + totalTexels - 1;

    uint32_t u0 = static_cast<uint32_t>(startTexel & 1023);
    uint32_t v0 = static_cast<uint32_t>(startTexel >> 10);
    uint32_t u1 = static_cast<uint32_t>(endTexel & 1023);
    uint32_t v1 = static_cast<uint32_t>(endTexel >> 10);

    if (v0 == v1) {
        // 单行内无跨行切分
        uint32_t count = u1 - u0 + 1;
        regions.push_back({u0, v0, count, 1, 0});
        return regions;
    }

    size_t currentTexelOffset = 0;

    // 跨越多行：至多分解为 3 个子区域
    // 步骤 A: 头部非满行
    if (u0 > 0) {
        uint32_t headCount = kWidth - u0;
        regions.push_back({u0, v0, headCount, 1, currentTexelOffset});
        currentTexelOffset += headCount;
        v0 += 1;
    }

    // 步骤 B: 中间连续完整行
    uint32_t fullRowEnd = (u1 == 1023) ? v1 : (v1 - 1);
    if (v0 <= fullRowEnd) {
        uint32_t fullRowCount = fullRowEnd - v0 + 1;
        regions.push_back({0, v0, kWidth, fullRowCount, currentTexelOffset});
        currentTexelOffset += static_cast<size_t>(fullRowCount) * kWidth;
    }

    // 步骤 C: 尾部非满行
    if (u1 < 1023) {
        uint32_t tailCount = u1 + 1;
        regions.push_back({0, v1, tailCount, 1, currentTexelOffset});
    }

    return regions;
}

void WGL2_StorageBuffer::updateData(const void* data, size_t size, size_t offset)
{
    if (size == 0 || !data) return;

    assert((offset % kTexelSizeBytes == 0) && "StorageBuffer 偏移必须按 16 字节对齐！");
    assert((size % kTexelSizeBytes == 0) && "StorageBuffer 尺寸必须按 16 字节对齐！");

    if (offset + size <= m_cpuShadowBuffer.size()) {
        std::memcpy(m_cpuShadowBuffer.data() + offset, data, size);
    }

    if (m_textureHandle == 0 || !glBindTexture || !glTexSubImage2D) return;

    const uint8_t* srcBytes = reinterpret_cast<const uint8_t*>(data);
    auto subRegions = calculateSubRegions(offset, size);

    glBindTexture(GL_TEXTURE_2D, m_textureHandle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    for (const auto& reg : subRegions) {
        const uint8_t* slicePtr = srcBytes + reg.texelOffset * kTexelSizeBytes;
        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        static_cast<GLint>(reg.x), static_cast<GLint>(reg.y),
                        static_cast<GLsizei>(reg.width), static_cast<GLsizei>(reg.height),
                        GL_RGBA_INTEGER, GL_UNSIGNED_INT, slicePtr);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace FCT::RHI
