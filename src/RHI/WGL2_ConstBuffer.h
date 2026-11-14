#pragma once

#include "./ConstBuffer.h"
#include "../Context/WGL2_ContextCommon.h"

namespace FCT {

class WGL2_Context;

namespace RHI {

/**
 * @brief WebGL 2.0 / GLES 3.0 常量缓冲 (UBO) (FEAT-26)
 * 支持帧环形大 UBO 批次暂存合并更新与独立 UBO 双模
 */
class WGL2_ConstBuffer : public ConstBuffer {
public:
    explicit WGL2_ConstBuffer(WGL2_Context* ctx);
    ~WGL2_ConstBuffer() override;

    void create() override;
    void create(const ConstLayout& layout);
    void upload(const void* data, size_t size) override;
    void upload(const void* data, size_t size, size_t offset) override;

    void bind(uint32_t slot);

    GLuint getBufferHandle() const noexcept;
    uint32_t getOffset() const noexcept { return m_offset; }
    uint32_t getSize() const noexcept { return m_size; }

private:
    WGL2_Context* m_ctx{nullptr};
    GLuint m_standaloneUbo{0};
    uint32_t m_offset{0};
    uint32_t m_size{0};
    bool m_useRingUBO{true};
    std::vector<uint8_t> m_localData;
};

} // namespace RHI
} // namespace FCT
