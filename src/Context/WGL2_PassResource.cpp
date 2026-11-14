#include "./WGL2_PassResource.h"
#include "./WGL2_Context.h"
#include "../RHI/WGL2_ConstBuffer.h"
#include "../RHI/WGL2_Sampler.h"
#include "../RHI/WGL2_Image.h"
#include "./Image.h"

namespace FCT {

WGL2_PassResource::WGL2_PassResource(WGL2_Context* ctx)
    : m_ctx(ctx)
{
}

void WGL2_PassResource::addConstBuffer(RHI::ConstBuffer* buffer)
{
    m_constBuffers.push_back(buffer);
}

void WGL2_PassResource::addSampler(Sampler* sampler, SamplerElement element)
{
    m_samplers.push_back({sampler, element});
}

void WGL2_PassResource::addTexture(Image* texture, TextureElement element)
{
    m_textures[element] = texture;
}

void WGL2_PassResource::setTexture(Image* texture, TextureElement element)
{
    m_textures[element] = texture;
}

void WGL2_PassResource::create()
{
}

void WGL2_PassResource::update()
{
}

void WGL2_PassResource::markAllDescriptorSetsNeedRecreate()
{
}

void WGL2_PassResource::bind(RHI::CommandBuffer* cmdBuf, RHI::Pipeline* pipeline)
{
    (void)cmdBuf;
    (void)pipeline;

    // 绑定常量缓冲区到 Uniform Buffer 槽位
    for (size_t i = 0; i < m_constBuffers.size(); ++i) {
        auto* cb = dynamic_cast<RHI::WGL2_ConstBuffer*>(m_constBuffers[i]);
        if (cb) {
            cb->bind(static_cast<uint32_t>(i));
        }
    }

    // 绑定纹理与采样器到各纹理单元
    uint32_t textureUnit = 0;
    for (auto& [elem, img] : m_textures) {
        if (!img) continue;
        auto* rhiImg = dynamic_cast<RHI::WGL2_Image*>(img->currentImage());
        if (rhiImg) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(rhiImg->getGLTarget(), rhiImg->getTextureHandle());

            // 查找关联的 Sampler
            for (auto& [smp, sElem] : m_samplers) {
                if (std::string(sElem.getName()) == std::string(elem.getName())) {
                    auto* wglSampler = dynamic_cast<RHI::WGL2_Sampler*>(smp);
                    if (wglSampler) {
                        wglSampler->bind(textureUnit);
                    }
                    break;
                }
            }
            textureUnit++;
        }
    }
}

} // namespace FCT
