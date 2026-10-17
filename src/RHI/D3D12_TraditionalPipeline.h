#ifndef FCT_D3D12_TRADITIONALPIPELINE_H
#define FCT_D3D12_TRADITIONALPIPELINE_H

#include "./D3D12_RasterizationPipeline.h"
#include "./D3D12_VertexShader.h"
#include "./D3D12_PixelShader.h"
#include "./D3D12_BlendState.h"
#include "./D3D12_RasterizationState.h"
#include "./D3D12_DepthStencilState.h"
#include "./D3D12_ViewportState.h"
#include "./D3D12_InputLayout.h"
#include "./D3D12_Pass.h"

namespace FCT
{
    class D3D12_Context;
    class VertexShader;
    class PixelShader;

    namespace RHI
    {
        struct D3D12_CbvSrvSlot
        {
            bool isCbv = false;
            uint32_t registerIndex = 0;
            ConstLayout constLayout;
            TextureElement textureElement;
        };

        struct D3D12_SamplerSlot
        {
            uint32_t registerIndex = 0;
            SamplerElement samplerElement;
        };

        struct D3D12_SetRootLayout
        {
            uint32_t cbvSrvRootParamIndex = UINT32_MAX;
            uint32_t samplerRootParamIndex = UINT32_MAX;
            std::vector<D3D12_CbvSrvSlot> cbvSrvSlots;
            std::vector<D3D12_SamplerSlot> samplerSlots;
        };

        class D3D12_TraditionalPipeline : public D3D12_RasterizationPipeline
        {
        public:
            explicit D3D12_TraditionalPipeline(D3D12_Context* ctx);
            ~D3D12_TraditionalPipeline() override;

            PipelineType getType() const override { return PipelineType::Traditional; }
            void addResources(IPipelineResource* resource) override;
            void create() override;
            void bindPass(Pass* pass) override;
            void bind(D3D12_CommandBuffer* cmdBuf) override;

            ID3D12RootSignature* getRootSignature() const { return m_rootSignature.Get(); }
            ID3D12PipelineState* getPipelineState() const { return m_pipelineState.Get(); }
            const D3D12_SetRootLayout& getSetLayout(uint32_t setIndex) const { return m_setLayouts[setIndex]; }

        private:
            void generateDefaultResources();
            void collectShaderResources(ShaderBinary binary);
            void createRootSignature();
            void createPipelineState();

        private:
            D3D12_Context* m_ctx;
            FCT::VertexShader* m_vertexShader;
            FCT::PixelShader* m_pixelShader;
            D3D12_BlendState* m_blendState;
            D3D12_RasterizationState* m_rasterizationState;
            D3D12_DepthStencilState* m_depthStencilState;
            D3D12_ViewportState* m_viewportState;
            D3D12_InputLayout* m_inputLayout;
            D3D12_Pass* m_pass;

            D3D12_SetRootLayout m_setLayouts[4];
            Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
            Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
        };
    }
}

#endif // FCT_D3D12_TRADITIONALPIPELINE_H
