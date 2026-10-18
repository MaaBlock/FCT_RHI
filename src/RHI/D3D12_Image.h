#ifndef FCT_D3D12_IMAGE_H
#define FCT_D3D12_IMAGE_H

#include "./Image.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_Image : public Image
        {
        public:
            explicit D3D12_Image(D3D12_Context* ctx);
            ~D3D12_Image() override;

            void create() override;
            void create(bool uploadData) override;
            void create(ID3D12Resource* externalResource, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_PRESENT);

            void updateData(const void* data, size_t dataSize) override;
            void updateData(const void* data, size_t dataSize, Fence* fence, std::function<void()>* onCompletion) override;
            void uploadAsync(std::vector<uint8_t> data, std::function<void()> callback) override;

            ID3D12Resource* getResource() const { return m_resource.Get(); }
            D3D12_RESOURCE_STATES getResourceState() const { return m_currentState; }
            void setResourceState(D3D12_RESOURCE_STATES state) { m_currentState = state; }
            D3D12_RESOURCE_STATES* getResourceStatePtr() { return &m_currentState; }

            void transitionState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

        private:
            D3D12_RESOURCE_FLAGS buildResourceFlags() const;
            DXGI_FORMAT resolveResourceFormat() const;
            D3D12_RESOURCE_STATES resolveInitialState() const;

        private:
            D3D12_Context* m_ctx;
            Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
            D3D12_RESOURCE_STATES m_currentState;
        };
    }
}

#endif // FCT_D3D12_IMAGE_H
