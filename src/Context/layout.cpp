#include "layout.h"
#include "layout.hpp"
#include "./PixelShader.h"
#include "./VertexShader.h"
#include "Context.hpp"
#include "PassResource.h"
#include "../RHI/BlendState.h"
#include "../RHI/DepthStencilState.h"
#include "../RHI/RasterizationPipeline.h"
#include "../RHI/RasterizationState.h"

namespace FCT
{
    void Layout::proccessArgs()
    {
        if (!m_hasPixelLayout && m_hasVertexLayout)
        {
            m_pixelLayout = m_vertexLayouts.begin()->second;
        }
    }

    void Layout::ctx(FCT::Context* ctx)
    {
        m_ctx = ctx;
    }

    void Layout::setFixedImage(std::string name, FCT::Image* image)
    {
        m_fixedImages[name] = image;
    }

    void Layout::addTextureSlot(FCT::TextureElement element)
    {
        if (m_resourceLayout.findTextureIndex(element.getName()) != -1) {
            return;
        }
        m_resourceLayout.addTexture(element);
        if (element.getShaderStages() & FCT::ShaderStage::Vertex)
        {
            m_vertexResourceLayout.addTexture(element);
        }
        if (element.getShaderStages() & FCT::ShaderStage::Fragment)
        {
            m_pixelResourceLayout.addTexture(element);
        }
        clearShaderCache();
        clearPipelineCache();
        clearPassResourceCache();
    }
    void Layout::addUniformSlot(const UniformSlot& uniformSlot)
    {
        if (m_uniformLayouts.count(uniformSlot.getName())) {
            return;
        }
        m_uniformLayouts[uniformSlot.getName()] = uniformSlot;
        clearShaderCache();
        clearPipelineCache();
        clearPassResourceCache();
    }

    void Layout::addTextureSlot(const FCT::TextureSlot& slot)
    {
        TextureElement element(
            slot.type,
            slot.name.c_str(),
            ShaderStage::All);
        addTextureSlot(element);
    }

    void Layout::addSamplerSlot(const SamplerSlot& samplerSlot)
    {
        auto existingSampler = m_resourceLayout.findSampler(samplerSlot.getName());
        if (existingSampler.getName() != nullptr && existingSampler.getName()[0] != '\0') {
            return;
        }

        m_resourceLayout.addSampler(samplerSlot);

        if (samplerSlot.getShaderStages() & FCT::ShaderStage::Vertex)
        {
            m_vertexResourceLayout.addSampler(samplerSlot);
        }
        if (samplerSlot.getShaderStages() & FCT::ShaderStage::Fragment)
        {
            m_pixelResourceLayout.addSampler(samplerSlot);
        }

        clearShaderCache();
        clearPipelineCache();
        clearPassResourceCache();

    }

    void Layout::addVertexLayout(const FCT::VertexLayout& vertexLayout)
    {
        uint32_t index = findNextAvailableIndex();
        m_vertexLayouts[index] = vertexLayout;
        m_hasVertexLayout = true;
        processUnhandledTextureSlots();
        clearShaderCache();
        clearPipelineCache();
    }

    void Layout::setPixelLayout(const FCT::PixelLayout& pixelLayout)
    {
        m_pixelLayout = pixelLayout;
        m_hasPixelLayout = true;
        processUnhandledTextureSlots();
        clearShaderCache();
        clearPipelineCache();
    }


    void Layout::removeTextureSlot(const char* name)
    {
        m_resourceLayout.removeTexture(name);

        m_vertexResourceLayout.removeTexture(name);

        m_pixelResourceLayout.removeTexture(name);

        auto it = m_fixedImages.find(name);
        if (it != m_fixedImages.end()) {
            m_fixedImages.erase(it);
        }

        // Note: 保留 m_textureNames 中的字符串池生命周期，避免 ShaderGenerator 缓存的 const char* 在重编译时悬垂
        clearShaderCache();
        clearPipelineCache();
        clearPassResourceCache();
    }

    Layout::~Layout()
    {
        if (m_ctx && !m_passName.empty())
        {
            if (m_passPipeSubIds.first != 0)
            {
                m_ctx->pipeHub().passPipe.unsubscribe(m_passName, m_passPipeSubIds.first);
                m_passPipeSubIds.first = 0;
            }
            if (m_passPipeSubIds.second != 0)
            {
                m_ctx->pipeHub().passPipe.unsubscribe(m_passName, m_passPipeSubIds.second);
                m_passPipeSubIds.second = 0;
            }
        }
    }

    void Layout::attachPass(std::string passName)
    {
        if (m_ctx && !m_passName.empty())
        {
            if (m_passPipeSubIds.first != 0)
            {
                m_ctx->pipeHub().passPipe.unsubscribe(m_passName, m_passPipeSubIds.first);
                m_passPipeSubIds.first = 0;
            }
            if (m_passPipeSubIds.second != 0)
            {
                m_ctx->pipeHub().passPipe.unsubscribe(m_passName, m_passPipeSubIds.second);
                m_passPipeSubIds.second = 0;
            }
        }
        m_passName = passName;
        m_passPipeSubIds = m_ctx->pipeHub()
             .passPipe.
             subscribe<PassInfo>(
                 passName,
                 [this](PassInfo& info)
                 {
                     m_pass = &info.pass;
                     m_textureFromPass = info.textures;
                     if (m_injectPassResource)
                     {
                         for (auto& texture : m_textureFromPass)
                         {
                             auto [nameIt, _] = m_textureNames.emplace(texture.first, nullptr);
                             nameIt->second = nameIt->first.c_str();
                             FCT::TextureElement element(
                                 TextureType::Texture2D,
                                 nameIt->second,
                                 getAllAfterTheStage(info.textureSlot[texture.first]),
                                 FCT::UpdateFrequency::PerFrame);
                             addTextureSlot(element);
                             setFixedImage(texture.first,
                                 texture.second);
                         }
                     }
                 },
                 [this]()
                 {
                     if (m_injectPassResource)
                     {
                         for (auto& texture : m_textureFromPass)
                         {
                             removeTextureSlot(texture.first.c_str());
                         }
                         clearShaderCache();
                         clearPipelineCache();
                     }
                     m_textureFromPass.clear();
                     clearPassResourceCache();
                 });
    }

    Uniform Layout::allocateUniform(std::string name)
    {
        return std::move(Uniform(m_ctx,m_uniformLayouts[name]));
    }

    FCT::VertexShader* Layout::allocateVertexShader(std::string code)
    {
        auto ret = m_ctx->createResource<FCT::ContextResource::VertexShader>();
        for (auto& layout : m_vertexLayouts)
        {
            ret->addLayout(layout.first,layout.second);
        }
        ret->pixelLayout(m_pixelLayout);
        for (auto& uniform : m_uniformLayouts)
        {
            ret->addUniform(uniform.second);
        }
        ret->resourceLayout(m_vertexResourceLayout);
        ret->code(code);
        ret->create();
        return ret;
    }

    FCT::PixelShader* Layout::allocatePixelShader(std::string code)
    {
        auto ret = m_ctx->createResource<FCT::ContextResource::PixelShader>();

        for (auto& layout : m_uniformLayouts)
        {
            ret->addUniform(layout.second);
        }
        ret->pixelLayout(m_pixelLayout);
        ret->resourceLayout(m_pixelResourceLayout);
        ret->code(code);
        ret->create();
        return ret;
    }

    void Layout::begin()
    {
        m_passResourceState.clear();

        for (const auto& fixedImage : m_fixedImages)
        {
            m_passResourceState.bindTexture(fixedImage.first, fixedImage.second);
        }

        m_pipelineState = TraditionPipelineState();
    }

    void Layout::bindUniform(const Uniform& uniform)
    {
        FCT::RHI::ConstBuffer* constBuffer = uniform;
        std::string name = constBuffer->layout().getName();
        m_passResourceState.bindUniform(name, uniform);
    }
    void Layout::bindUniform(Uniform* uniform)
    {
        m_passResourceState.bindUniform(uniform->getConstBuffer()->layout().getName(), uniform);
    }

    void Layout::bindTexture(std::string name, FCT::Image* image)
    {
        m_passResourceState.bindTexture(name, image);
    }

    void Layout::bindSampler(std::string name, FCT::Sampler* sampler)
    {
        m_passResourceState.bindSampler(name, sampler);
    }

    void Layout::bindVertexShader(FCT::VertexShader* shader)
    {
        m_pipelineState.vertexShader = shader;
    }

    void Layout::bindPixelShader(FCT::PixelShader* shader)
    {
        m_pipelineState.pixelShader = shader;
    }

    void Layout::bindDepthStencilState(FCT::DepthStencilState* state)
    {
        m_pipelineState.depthStencilState = state;
    }

    void Layout::bindBlendState(FCT::BlendState* state)
    {
        m_pipelineState.blendState = state;
    }

    void Layout::bindRasterizationState(FCT::RasterizationState* state)
    {
        m_pipelineState.rasterizationState = state;
    }


    void Layout::end()
    {

    }

    size_t Layout::TraditionPipelineState::hash() const
    {
        size_t hash = 0;
        boost::hash_combine(hash, vertexShader);
        boost::hash_combine(hash, pixelShader);
        boost::hash_combine(hash, blendState);
        boost::hash_combine(hash, rasterizationState);
        boost::hash_combine(hash, depthStencilState);
        return hash;
    }

    bool Layout::TraditionPipelineState::operator==(const TraditionPipelineState& other) const
    {
        return vertexShader == other.vertexShader &&
            pixelShader == other.pixelShader &&
            blendState == other.blendState &&
            rasterizationState == other.rasterizationState &&
            depthStencilState == other.depthStencilState;
    }

    void Layout::PassResourceState::clear()
    {
        boundTextures.clear();
        boundSamplers.clear();
        boundUniforms.clear();
    }

    void Layout::PassResourceState::bindTexture(const std::string& name, FCT::Image* image)
    {
        boundTextures[name] = image;
    }

    void Layout::PassResourceState::bindSampler(const std::string& name, FCT::Sampler* sampler)
    {
        boundSamplers[name] = sampler;
    }

    void Layout::PassResourceState::bindUniform(const std::string& name, const Uniform& uniform)
    {
        boundUniforms[name] = uniform;
    }
    void Layout::PassResourceState::bindUniform(const std::string& name, Uniform* uniform)
    {
        boundUniforms[name] = uniform->getConstBuffer();
    }

    size_t Layout::PassResourceState::hash() const
    {
        size_t hash = 0;

        for (const auto& tex : boundTextures) {
            boost::hash_combine(hash, tex.first);
            boost::hash_combine(hash, tex.second);
            if (tex.second) {
                boost::hash_combine(hash, tex.second->textureViewHash());
            }
        }

        for (const auto& samp : boundSamplers) {
            boost::hash_combine(hash, samp.first);
            boost::hash_combine(hash, samp.second);
        }

        for (const auto& uni : boundUniforms) {
            boost::hash_combine(hash, uni.first);
            boost::hash_combine(hash, uni.second);
        }

        return hash;
    }

    Layout::PassResourceCache::~PassResourceCache()
    {
        for (auto& pair : m_passResources)
        {
            pair.second->release();
        }
    }

    FCT::PassResource* Layout::PassResourceCache::get(const PassResourceState& state,
                                                      const std::function<FCT::PassResource*(const PassResourceState& state)>& creator)
    {
        auto hash = state.hash();
        if (m_passResources.count(hash))
        {
            return m_passResources[hash];
        } else
        {
            auto passResource = creator(state);
            m_passResources[hash] = passResource;
            return passResource;
        }
    }

    Layout::PipelineCache::~PipelineCache()
    {
        for (auto& pair : m_pipelines)
        {
            pair.second->release();
        }
    }

    FCT::RHI::RasterizationPipeline* Layout::PipelineCache::get(const TraditionPipelineState& state,
                                                                const std::function<FCT::RHI::RasterizationPipeline*(const TraditionPipelineState& state)>& creator)
    {
        auto hash = state.hash();
        if (m_pipelines.count(hash))
        {
            return m_pipelines[hash];
        } else
        {
            auto pipeline = creator(state);
            m_pipelines[hash] = pipeline;
            return pipeline;
        }
    }

    void Layout::processUnhandledTextureSlots()
    {
        auto it = m_unhandledTextureSlots.begin();
        while (it != m_unhandledTextureSlots.end())
        {
            FCT::ShaderStages usage;

            if (!m_hasVertexLayout && !m_hasPixelLayout)
            {
                ++it;
                continue;
            }
            else if (m_hasVertexLayout && !m_hasPixelLayout)
            {
                usage = FCT::ShaderStage::Vertex | FCT::ShaderStage::Fragment;
            }
            else if (!m_hasVertexLayout && m_hasPixelLayout)
            {
                usage = FCT::ShaderStage::Vertex | FCT::ShaderStage::Fragment;
            }
            else
            {
                usage = FCT::ShaderStage::Fragment;
            }
            m_textureNames[it->name] = nullptr;
            m_textureNames[it->name] =  m_textureNames.find(it->name)->first.c_str();
            FCT::TextureElement element(it->type, m_textureNames[it->name], usage, FCT::UpdateFrequency::PerFrame);
            addTextureSlot(element);

            it = m_unhandledTextureSlots.erase(it);
        }
    }

    uint32_t Layout::findNextAvailableIndex()
    {
        while (m_vertexLayouts.find(m_nextAvailableIndex) != m_vertexLayouts.end()) {
            ++m_nextAvailableIndex;
        }
        return m_nextAvailableIndex++;
    }

    FCT::PassResource* Layout::getCurrentPassResource()
    {
        return m_passResourceCache.get(m_passResourceState, [this](const PassResourceState& state)
        {
            auto ret = m_ctx->createResource<FCT::PassResource>();

            for (const auto& layoutPair : m_uniformLayouts) {
                const std::string& name = layoutPair.first;
                const UniformSlot& slot = layoutPair.second;
                auto boundIt = state.boundUniforms.find(name);
                if (boundIt != state.boundUniforms.end()) {
                    ret->addConstBuffer(boundIt->second);
                } else {
                    ret->addConstBuffer(m_ctx->getEmptyConstBuffer(slot));
                }
            }

            for (size_t i = 0; i < m_resourceLayout.getTextureCount(); ++i) {
                const TextureElement& element = m_resourceLayout.getTexture(i);
                auto boundIt = state.boundTextures.find(element.getName());
                if (boundIt != state.boundTextures.end()) {
                    ret->addTexture(boundIt->second, element);
                } else {
                    ret->addTexture(nullptr, element);
                }
            }

            for (size_t i = 0; i < m_resourceLayout.getSamplerCount(); ++i) {
                const SamplerElement& element = m_resourceLayout.getSampler(i);
                auto boundIt = state.boundSamplers.find(element.getName());
                if (boundIt != state.boundSamplers.end()) {
                    ret->addSampler(boundIt->second, element);
                } else {
                    ret->addSampler(m_ctx->getEmptySampler(), element);
                }
            }

            ret->create();
            return ret;
        });
    }

    FCT::RHI::RasterizationPipeline* Layout::getCurrentPipeline()
    {
        return m_pipelineCache.get(m_pipelineState, [this](const TraditionPipelineState& state)
        {
            auto ret = m_ctx->createTraditionPipeline();

            for (auto& layout : m_vertexLayouts)
            {
                ret->vertexLayout(layout.second);
            }

            ret->pixelLayout(m_pixelLayout);

            if (state.vertexShader) {
                ret->addResources(state.vertexShader);
            }
            if (state.pixelShader) {
                ret->addResources(state.pixelShader);
            }

            if (state.blendState) {
                ret->addResources(state.blendState);
            }
            if (state.rasterizationState) {
                ret->addResources(state.rasterizationState);
            }
            if (state.depthStencilState) {
                ret->addResources(state.depthStencilState);
            }

            if (m_pass) {
                ret->bindPass(m_pass);
            }

            Status st = ret->create();
            if (!st.ok()) {
                ferr << "[Layout] 管线创建失败: " << st.message() << std::endl;
            }
            return ret;
        });
    }


    Layout::ShaderCache::~ShaderCache()
    {
        for (auto& pair : m_vertexShaders) {
            pair.second->release();
        }
        for (auto& pair : m_pixelShaders) {
            pair.second->release();
        }
    }

    FCT::VertexShader* Layout::ShaderCache::getVertexShader(const std::string& code,
                                                            const std::function<FCT::VertexShader*(const std::string& code)>& creator)
    {
        auto hash = std::hash<std::string>{}(code);
        if (m_vertexShaders.count(hash))
        {
            return m_vertexShaders[hash];
        }
        else
        {
            auto shader = creator(code);
            m_vertexShaders[hash] = shader;
            return shader;
        }
    }

    FCT::PixelShader* Layout::ShaderCache::getPixelShader(const std::string& code,
        const std::function<FCT::PixelShader*(const std::string& code)>& creator)
    {
        auto hash = std::hash<std::string>{}(code);
        if (m_pixelShaders.count(hash))
        {
            return m_pixelShaders[hash];
        }
        else
        {
            auto shader = creator(code);
            m_pixelShaders[hash] = shader;
            return shader;
        }
    }

    FCT::VertexShader* Layout::ShaderCache::getVertexShader(const ShaderRef& ref,
        const std::function<FCT::VertexShader*(const ShaderRef& ref)>& creator)
    {
        auto hash = ref.hash;
        if (m_vertexShaders.count(hash))
        {
            return m_vertexShaders[hash];
        }
        else
        {
            auto shader = creator(ref);
            m_vertexShaders[hash] = shader;
            return shader;
        }
    }

    FCT::PixelShader* Layout::ShaderCache::getPixelShader(const ShaderRef& ref,
        const std::function<FCT::PixelShader*(const ShaderRef& ref)>& creator)
    {
        auto hash = ref.hash;
        if (m_pixelShaders.count(hash))
        {
            return m_pixelShaders[hash];
        }
        else
        {
            auto shader = creator(ref);
            m_pixelShaders[hash] = shader;
            return shader;
        }
    }


    VertexShader* Layout::getCacheVertexShader(std::string code)
    {
        return m_shaderCache.getVertexShader(code, [this](const std::string& code) -> FCT::VertexShader*
        {
            return allocateVertexShader(code);
        });
    }

    PixelShader* Layout::getCachePixelShader(std::string code)
    {
        return m_shaderCache.getPixelShader(code, [this](const std::string& code) -> FCT::PixelShader*
        {
            return allocatePixelShader(code);
        });
    }

    VertexShader* Layout::getCacheVertexShader(const ShaderRef& ref)
    {
        return m_shaderCache.getVertexShader(ref, [this](const ShaderRef& ref) -> FCT::VertexShader*
        {
            return allocateVertexShader(ref.code);
        });
    }

    PixelShader* Layout::getCachePixelShader(const ShaderRef& ref)
    {
        return m_shaderCache.getPixelShader(ref, [this](const ShaderRef& ref) -> FCT::PixelShader*
        {
            return allocatePixelShader(ref.code);
        });
    }

    ShaderRef Layout::cacheVertexShader(const std::string& code)
    {
        ShaderRef ref;
        ref.code = code;
        ref.hash = std::hash<std::string>{}(code);
        ref.kind = FCT::ShaderKind::VertexShader;
        getCacheVertexShader(ref);
        return ref;
    }

    ShaderRef Layout::cachePixelShader(const std::string& code)
    {
        ShaderRef ref;
        ref.code = code;
        ref.hash = std::hash<std::string>{}(code);
        ref.kind = FCT::ShaderKind::FragmentShader;
        getCachePixelShader(ref);
        return ref;
    }

    void Layout::bindVertexShader(std::string code)
    {
        auto shader = getCacheVertexShader(code);
        bindVertexShader(shader);
    }

    void Layout::bindPixelShader(std::string code)
    {
        auto shader = getCachePixelShader(code);
        bindPixelShader(shader);
    }

    void Layout::bindVertexShader(const ShaderRef& ref)
    {
        auto shader = getCacheVertexShader(ref);
        bindVertexShader(shader);
    }

    void Layout::bindPixelShader(const ShaderRef& ref)
    {
        auto shader = getCachePixelShader(ref);
        bindPixelShader(shader);
    }
    void Layout::ShaderCache::clear()
    {
        for (auto& shader : m_vertexShaders) {
            if (shader.second) {
                shader.second->release();
            }
        }
        for (auto& shader : m_pixelShaders) {
            if (shader.second) {
                shader.second->release();
            }
        }
        m_vertexShaders.clear();
        m_pixelShaders.clear();
    }

    void Layout::clearPassResourceCache()
    {
        for (auto& resource : m_passResourceCache.m_passResources) {
            if (resource.second) {
                resource.second->release();
            }
        }
        m_passResourceCache.m_passResources.clear();
    }

    void Layout::clearPipelineCache()
    {
        for (auto& pipeline : m_pipelineCache.m_pipelines) {
            if (pipeline.second) {
                pipeline.second->release();
            }
        }
        m_pipelineCache.m_pipelines.clear();
    }

    void Layout::clearShaderCache()
    {
        m_shaderCache.clear();
    }
}
