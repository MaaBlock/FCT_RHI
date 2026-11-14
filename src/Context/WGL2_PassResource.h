#pragma once

#include "./WGL2_ContextCommon.h"
#include "./Image.h"
#include "./PassResource.h"

namespace FCT {

class WGL2_Context;

class WGL2_PassResource : public PassResource {
public:
    explicit WGL2_PassResource(WGL2_Context* ctx);
    ~WGL2_PassResource() override = default;

    void addConstBuffer(RHI::ConstBuffer* buffer) override;
    void addSampler(Sampler* sampler, SamplerElement element) override;
    void addTexture(Image* texture, TextureElement element) override;
    void setTexture(Image* texture, TextureElement element) override;

    void create() override;
    void update() override;
    void bind(RHI::CommandBuffer* cmdBuf, RHI::Pipeline* pipeline) override;
    void markAllDescriptorSetsNeedRecreate() override;

private:
    WGL2_Context* m_ctx{nullptr};
};

} // namespace FCT
