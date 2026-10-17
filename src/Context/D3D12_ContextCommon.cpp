#include "../FCTAPI.h"
#include "./D3D12_ContextCommon.h"
#include "./D3D12_Context.h"

namespace FCT
{
    D3D12_ContextCommon::D3D12_ContextCommon(Runtime* runtime)
        : m_runtime(runtime)
        , m_pickAdapterId(0)
    {
    }

    D3D12_ContextCommon::~D3D12_ContextCommon()
    {
        term();
    }

    void D3D12_ContextCommon::enableDebugLayer()
    {
#ifdef FCT_DEBUG
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
#endif
    }

    void D3D12_ContextCommon::enumerateAdapters()
    {
        m_adapters.clear();
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        for (UINT index = 0;
             SUCCEEDED(m_factory->EnumAdapterByGpuPreference(
                 index,
                 DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                 IID_PPV_ARGS(&adapter)));
             ++index)
        {
            DXGI_ADAPTER_DESC1 desc{};
            adapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                m_adapters.push_back(adapter);
            }
        }

        if (m_adapters.empty())
        {
            for (UINT index = 0; m_factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND; ++index)
            {
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                {
                    m_adapters.push_back(adapter);
                }
            }
        }
    }

    void D3D12_ContextCommon::init()
    {
        enableDebugLayer();

        UINT dxgiFactoryFlags = 0;
#ifdef FCT_DEBUG
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
        HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory));
        if (FAILED(hr))
        {
            ferr << "Failed to create DXGI Factory, hr = " << std::hex << hr << std::dec << std::endl;
            return;
        }

        enumerateAdapters();
    }

    void D3D12_ContextCommon::term()
    {
        m_adapters.clear();
        m_factory.Reset();
    }

    D3D12_Context* D3D12_ContextCommon::createContext()
    {
        return new D3D12_Context(this);
    }

    IDXGIAdapter1* D3D12_ContextCommon::getAdapter() const
    {
        if (m_pickAdapterId < m_adapters.size())
        {
            return m_adapters[m_pickAdapterId].Get();
        }
        return nullptr;
    }
}
