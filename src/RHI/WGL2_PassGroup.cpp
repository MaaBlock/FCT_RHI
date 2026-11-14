#include "./WGL2_PassGroup.h"
#include "./WGL2_Pass.h"
#include "./WGL2_Image.h"
#include "../Context/WGL2_Context.h"
#include "./CommandBuffer.h"
#include <iostream>

namespace FCT::RHI {

WGL2_PassGroup::WGL2_PassGroup(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

WGL2_PassGroup::~WGL2_PassGroup()
{
    for (auto& [hash, fbo] : m_fboCache) {
        if (fbo != 0) {
            glDeleteFramebuffers(1, &fbo);
        }
    }
    m_fboCache.clear();
}

void WGL2_PassGroup::create()
{
    for (auto* pass : m_passes) {
        if (pass) {
            pass->create(this);
        }
    }
}

uint64_t WGL2_PassGroup::computeAttachmentHash(const std::vector<WGL2_Image*>& colorTargets, WGL2_Image* depthStencil) noexcept
{
    // 64-bit FNV-1a 哈希
    uint64_t hash = 14695981039346656037ULL;
    auto hashCombine = [&hash](uint64_t val) {
        hash ^= val;
        hash *= 1099511628211ULL;
    };

    hashCombine(static_cast<uint64_t>(colorTargets.size()));
    for (auto* ct : colorTargets) {
        uint64_t ptrVal = reinterpret_cast<uint64_t>(ct);
        hashCombine(ptrVal);
        if (ct) {
            hashCombine(static_cast<uint64_t>(ct->getTextureHandle()));
            hashCombine(static_cast<uint64_t>(ct->isRenderbuffer()));
        }
    }

    uint64_t dsPtr = reinterpret_cast<uint64_t>(depthStencil);
    hashCombine(dsPtr);
    if (depthStencil) {
        hashCombine(static_cast<uint64_t>(depthStencil->getTextureHandle()));
        hashCombine(static_cast<uint64_t>(depthStencil->isRenderbuffer()));
    }

    return hash;
}

std::vector<GLenum> WGL2_PassGroup::translateAttachmentsForInvalidate(const std::vector<GLenum>& attachments, bool isDefaultFBO)
{
    if (attachments.empty()) {
        return {};
    }

    std::vector<GLenum> mappedAttachments;
    mappedAttachments.reserve(attachments.size() + 2);

    for (GLenum att : attachments) {
        if (isDefaultFBO) {
            // FEAT-35: FBO 0 语义逐元素映射：GL_DEPTH_ATTACHMENT -> GL_DEPTH，GL_COLOR_ATTACHMENTi -> GL_COLOR
            if (att >= GL_COLOR_ATTACHMENT0 && att <= GL_COLOR_ATTACHMENT15) {
                mappedAttachments.push_back(GL_COLOR);
            } else if (att == GL_DEPTH_ATTACHMENT) {
                mappedAttachments.push_back(GL_DEPTH);
            } else if (att == GL_STENCIL_ATTACHMENT) {
                mappedAttachments.push_back(GL_STENCIL);
            } else if (att == GL_DEPTH_STENCIL_ATTACHMENT) {
                mappedAttachments.push_back(GL_DEPTH);
                mappedAttachments.push_back(GL_STENCIL);
            } else {
                mappedAttachments.push_back(att);
            }
        } else {
            mappedAttachments.push_back(att);
        }
    }
    return mappedAttachments;
}

void WGL2_PassGroup::emitInvalidate(const std::vector<GLenum>& attachments, bool isDefaultFBO)
{
    std::vector<GLenum> mappedAttachments = translateAttachmentsForInvalidate(attachments, isDefaultFBO);
    if (!mappedAttachments.empty()) {
#if !defined(__EMSCRIPTEN__)
        if (glInvalidateFramebuffer == nullptr) {
            return;
        }
#endif
        std::vector<GLenum> uniqueAttachments;
        uniqueAttachments.reserve(mappedAttachments.size());
        for (GLenum att : mappedAttachments) {
            bool exists = false;
            for (GLenum existing : uniqueAttachments) {
                if (existing == att) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                uniqueAttachments.push_back(att);
            }
        }
        glInvalidateFramebuffer(GL_FRAMEBUFFER, static_cast<GLsizei>(uniqueAttachments.size()), uniqueAttachments.data());
    }
}

GLuint WGL2_PassGroup::getOrCreateFBO(uint64_t hashKey, const std::vector<WGL2_Image*>& colorTargets, WGL2_Image* depthStencil)
{
    auto it = m_fboCache.find(hashKey);
    if (it != m_fboCache.end()) {
        return it->second;
    }

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    std::vector<GLenum> drawBuffers;
    for (size_t i = 0; i < colorTargets.size(); ++i) {
        auto* ct = colorTargets[i];
        if (!ct) continue;

        GLenum attachmentPoint = static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i);
        if (ct->isRenderbuffer()) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachmentPoint, GL_RENDERBUFFER, ct->getTextureHandle());
        } else {
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, ct->getGLTarget(), ct->getTextureHandle(), 0);
        }
        drawBuffers.push_back(attachmentPoint);
    }

    if (!drawBuffers.empty()) {
        glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
    } else {
        glDrawBuffers(0, nullptr);
    }

    if (depthStencil) {
        Format fmt = depthStencil->format();
        GLenum attachmentPoint = GL_DEPTH_ATTACHMENT;
        if (fmt == Format::D24_UNORM_S8_UINT || fmt == Format::D32_SFLOAT_S8_UINT) {
            attachmentPoint = GL_DEPTH_STENCIL_ATTACHMENT;
        }

        if (depthStencil->isRenderbuffer()) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachmentPoint, GL_RENDERBUFFER, depthStencil->getTextureHandle());
        } else {
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, depthStencil->getGLTarget(), depthStencil->getTextureHandle(), 0);
        }
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[WGL2_PassGroup] Framebuffer is incomplete! Status: 0x" << std::hex << status << std::dec << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_fboCache[hashKey] = fbo;
    return fbo;
}

void WGL2_PassGroup::bindCurrentFBO()
{
    if (m_passes.empty()) {
        return;
    }

    // 收集第 0 个 pass 的附件（在同一 PassGroup 中附件物理拓扑必须兼容）
    auto* firstPass = m_passes[0];
    std::vector<WGL2_Image*> colorTargets;
    WGL2_Image* depthStencil = nullptr;

    bool isDefault = m_tbdrPlan.isDefaultFBO;

    for (size_t i = 0; i < 8; ++i) {
        // Pass 中查找索引
        auto it = firstPass->renderTargets().find(static_cast<uint32_t>(i));
        if (it != firstPass->renderTargets().end() && it->second != nullptr) {
            auto* img = dynamic_cast<WGL2_Image*>(it->second->currentImage());
            if (img) {
                colorTargets.push_back(img);
            }
        }
    }

    if (firstPass->depthStencil() != nullptr) {
        depthStencil = dynamic_cast<WGL2_Image*>(firstPass->depthStencil()->currentImage());
    }

    if (colorTargets.empty() && depthStencil == nullptr) {
        isDefault = true;
    }

    if (isDefault) {
        m_activeFBO = 0;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        uint64_t hashKey = computeAttachmentHash(colorTargets, depthStencil);
        m_activeFBO = getOrCreateFBO(hashKey, colorTargets, depthStencil);
        glBindFramebuffer(GL_FRAMEBUFFER, m_activeFBO);
    }
}

void WGL2_PassGroup::beginSubmit(CommandBuffer* cmdBuf)
{
    m_currentPassIndex = 0;
    bindCurrentFBO();

    // FEAT-35: 批次开始 TBDR Invalidate 发射
    emitInvalidate(m_tbdrPlan.beginDiscards, m_tbdrPlan.isDefaultFBO);

    if (!m_passes.empty() && m_passes[0] != nullptr) {
        m_passes[0]->beginSubmit(cmdBuf);
        m_passes[0]->executeClear(cmdBuf);
    }
}

void WGL2_PassGroup::nextPass(CommandBuffer* cmdBuf)
{
    if (m_currentPassIndex < m_passes.size() && m_passes[m_currentPassIndex] != nullptr) {
        m_passes[m_currentPassIndex]->endSubmit();
    }

    m_currentPassIndex++;

    if (m_currentPassIndex < m_passes.size() && m_passes[m_currentPassIndex] != nullptr) {
        m_passes[m_currentPassIndex]->beginSubmit(cmdBuf);
        m_passes[m_currentPassIndex]->executeClear(cmdBuf);
    }
}

void WGL2_PassGroup::endSubmit(CommandBuffer* cmdBuf)
{
    (void)cmdBuf;
    if (m_currentPassIndex < m_passes.size() && m_passes[m_currentPassIndex] != nullptr) {
        m_passes[m_currentPassIndex]->endSubmit();
    }

    // FEAT-35: 批次结束解绑 FBO 前 TBDR Invalidate 发射
    emitInvalidate(m_tbdrPlan.endDiscards, m_tbdrPlan.isDefaultFBO);

    // 解绑 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_activeFBO = 0;
}

} // namespace FCT::RHI
