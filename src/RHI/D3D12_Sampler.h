#ifndef FCT_D3D12_SAMPLER_H
#define FCT_D3D12_SAMPLER_H

#include "./Sampler.h"

namespace FCT
{
    class D3D12_Context;

    class D3D12_Sampler : public Sampler
    {
    public:
        explicit D3D12_Sampler(D3D12_Context* ctx);
        ~D3D12_Sampler() override;

        void create() override;

        D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle() const { return m_cpuHandle; }
        bool isValid() const { return m_descriptorIndex != UINT32_MAX; }

    private:
        void releaseDescriptor();
        D3D12_FILTER toD3D12Filter() const;
        D3D12_TEXTURE_ADDRESS_MODE toD3D12AddressMode(AddressMode mode) const;
        D3D12_COMPARISON_FUNC toD3D12CompareFunc(CompareOp op) const;
        void fillBorderColor(float outBorder[4]) const;

    private:
        D3D12_Context* m_ctx;
        D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle;
        uint32_t m_descriptorIndex;
    };
}

#endif // FCT_D3D12_SAMPLER_H
