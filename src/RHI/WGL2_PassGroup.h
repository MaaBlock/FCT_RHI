#pragma once

#include "./PassGroup.h"
#include "../Context/WGL2_ContextCommon.h"
#include <unordered_map>
#include <vector>

namespace FCT {

class WGL2_Context;

namespace RHI {

class WGL2_Pass;

/**
 * @brief TBDR 无效化计划 (FEAT-35, FEAT-36)
 */
struct TBDRInvalidatePlan {
    bool isDefaultFBO{false};
    std::vector<GLenum> beginDiscards;
    std::vector<GLenum> endDiscards;
};

/**
 * @brief WebGL 2.0 / GLES 3.0 PassGroup 实现 (FEAT-28, FEAT-35, FEAT-36)
 * 支持 64 位 FBO 物理缓存、FBO 0 语义映射与 TBDR glInvalidateFramebuffer 自动发射
 */
class WGL2_PassGroup : public PassGroup {
public:
    explicit WGL2_PassGroup(WGL2_Context* ctx);
    ~WGL2_PassGroup() override;

    void create() override;
    void beginSubmit(CommandBuffer* cmdBuf) override;
    void nextPass(CommandBuffer* cmdBuf) override;
    void endSubmit(CommandBuffer* cmdBuf) override;

    void setTBDRInvalidatePlan(const TBDRInvalidatePlan& plan) noexcept { m_tbdrPlan = plan; }
    const TBDRInvalidatePlan& getTBDRInvalidatePlan() const noexcept { return m_tbdrPlan; }

    GLuint getOrCreateFBO(uint64_t hashKey, const std::vector<class WGL2_Image*>& colorTargets, class WGL2_Image* depthStencil);

    static uint64_t computeAttachmentHash(const std::vector<class WGL2_Image*>& colorTargets, class WGL2_Image* depthStencil) noexcept;
    static std::vector<GLenum> translateAttachmentsForInvalidate(const std::vector<GLenum>& attachments, bool isDefaultFBO);
    static void emitInvalidate(const std::vector<GLenum>& attachments, bool isDefaultFBO);

private:
    void bindCurrentFBO();

    WGL2_Context* m_ctx{nullptr};
    size_t m_currentPassIndex{0};
    GLuint m_activeFBO{0};
    TBDRInvalidatePlan m_tbdrPlan;

    std::unordered_map<uint64_t, GLuint> m_fboCache;
};

} // namespace RHI
} // namespace FCT
