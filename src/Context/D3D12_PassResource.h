#ifndef FCT_D3D12_PASSRESOURCE_H
#define FCT_D3D12_PASSRESOURCE_H

#include "./PassResource.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_TraditionalPipeline;
    }

    struct D3D12_CachedTableSlice
    {
        bool allocated = false;
        bool dirty = true;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{0};
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{0};
        uint32_t baseIndex = 0;
        uint32_t count = 0;
    };

    struct D3D12_PipelineFrameTables
    {
        D3D12_CachedTableSlice cbvSrvTable[4];
        D3D12_CachedTableSlice samplerTable[4];
    };

    class D3D12_PassResource : public PassResource
    {
    public:
        explicit D3D12_PassResource(D3D12_Context* ctx);
        ~D3D12_PassResource() override = default;

        void addConstBuffer(RHI::ConstBuffer* buffer) override;
        void addSampler(Sampler* sampler, SamplerElement element) override;
        void addTexture(Image* texture, TextureElement element) override;
        void setTexture(Image* texture, TextureElement element) override;

        void create() override;
        void update() override;
        void bind(RHI::CommandBuffer* cmdBuf, RHI::Pipeline* pipeline) override;

        void markAllDescriptorSetsNeedRecreate();

    private:
        void updateAndBindSetTables(
            ID3D12GraphicsCommandList* cmdList,
            RHI::D3D12_TraditionalPipeline* d3dPipeline,
            uint32_t frameIdx);

    private:
        D3D12_Context* m_ctx;
        std::unordered_map<RHI::D3D12_TraditionalPipeline*, std::vector<D3D12_PipelineFrameTables>> m_pipelineTables;
    };
}

#endif // FCT_D3D12_PASSRESOURCE_H
