#ifndef FCT_D3D12_RASTERIZATIONPIPELINE_H
#define FCT_D3D12_RASTERIZATIONPIPELINE_H

#include "./RasterizationPipeline.h"
#include "./D3D12_CommandBuffer.h"

namespace FCT
{
    namespace RHI
    {
        class D3D12_RasterizationPipeline : public RasterizationPipeline
        {
        public:
            D3D12_RasterizationPipeline() = default;
            ~D3D12_RasterizationPipeline() override = default;

            void bind(CommandBuffer* cmdBuf) override
            {
                if (auto* d3dCmd = dynamic_cast<D3D12_CommandBuffer*>(cmdBuf))
                {
                    bind(d3dCmd);
                }
            }

            virtual void bind(D3D12_CommandBuffer* cmdBuf) = 0;
        };
    }
}

#endif // FCT_D3D12_RASTERIZATIONPIPELINE_H
