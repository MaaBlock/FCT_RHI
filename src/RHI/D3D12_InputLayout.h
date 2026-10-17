#ifndef FCT_D3D12_INPUTLAYOUT_H
#define FCT_D3D12_INPUTLAYOUT_H

#include "./InputLayout.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_InputLayout : public InputLayout
        {
        public:
            explicit D3D12_InputLayout(D3D12_Context* ctx);
            ~D3D12_InputLayout() override = default;

            void create() override;

            D3D12_INPUT_LAYOUT_DESC getInputLayoutDesc() const;
            const std::vector<D3D12_INPUT_ELEMENT_DESC>& getInputElements() const { return m_inputElements; }

        private:
            static void splitSemantic(const char* rawSemantic, std::string& outName, UINT& outIndex);

        private:
            D3D12_Context* m_ctx;
            std::vector<std::string> m_semanticNames;
            std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputElements;
        };
    }
}

#endif // FCT_D3D12_INPUTLAYOUT_H
