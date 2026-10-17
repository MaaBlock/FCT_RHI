#ifndef FCT_D3D12_DEPTHSTENCILVIEW_H
#define FCT_D3D12_DEPTHSTENCILVIEW_H

#include "./DepthStencilView.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_DepthStencilView : public DepthStencilView
        {
        public:
            explicit D3D12_DepthStencilView(D3D12_Context* ctx);
            ~D3D12_DepthStencilView() override;

            bool create() override;

            D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle() const { return m_cpuHandle; }
            bool isValid() const { return m_descriptorIndex != UINT32_MAX; }

        private:
            void releaseDescriptor();

        private:
            D3D12_Context* m_ctx;
            D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle;
            uint32_t m_descriptorIndex;
        };
    }
}

#endif // FCT_D3D12_DEPTHSTENCILVIEW_H
