#ifndef FCT_D3D12_CONTEXTCOMMON_H
#define FCT_D3D12_CONTEXTCOMMON_H

#include "../ThirdParty.h"
#include "../DebugTools/OutStream.h"

namespace FCT
{
    class Runtime;
    class D3D12_Context;

    class D3D12_ContextCommon
    {
    public:
        explicit D3D12_ContextCommon(Runtime* runtime);
        ~D3D12_ContextCommon();

        void init();
        void term();

        D3D12_Context* createContext();

        IDXGIAdapter1* getAdapter() const;
        IDXGIFactory6* getFactory() const { return m_factory.Get(); }
        Runtime* runtime() const { return m_runtime; }

    private:
        void enableDebugLayer();
        void enumerateAdapters();

    private:
        Runtime* m_runtime;
        size_t m_pickAdapterId;
        Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
        std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> m_adapters;
    };
}

#endif // FCT_D3D12_CONTEXTCOMMON_H
