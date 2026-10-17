#ifndef FCT_D3D12_RASTERIZATIONPIPELINE_H
#define FCT_D3D12_RASTERIZATIONPIPELINE_H

#include "./RasterizationPipeline.h"

namespace FCT
{
    namespace RHI
    {
        class D3D12_CommandBuffer;

        class D3D12_RasterizationPipeline : public RasterizationPipeline
        {
        public:
            D3D12_RasterizationPipeline() = default;
            ~D3D12_RasterizationPipeline() override = default;

            virtual void bind(D3D12_CommandBuffer* cmdBuf) = 0;
        };
    }
}

#endif // FCT_D3D12_RASTERIZATIONPIPELINE_H
