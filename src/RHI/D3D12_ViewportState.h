#ifndef FCT_D3D12_VIEWPORTSTATE_H
#define FCT_D3D12_VIEWPORTSTATE_H

#include "./ViewportState.h"

namespace FCT
{
    class D3D12_Context;

    class D3D12_ViewportState : public ViewportState
    {
    public:
        explicit D3D12_ViewportState(D3D12_Context* ctx);
        ~D3D12_ViewportState() override = default;

        void create() override;

    private:
        D3D12_Context* m_ctx;
    };
}

#endif // FCT_D3D12_VIEWPORTSTATE_H
