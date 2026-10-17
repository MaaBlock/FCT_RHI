#ifndef FCT_D3D12_VERTEXSHADER_H
#define FCT_D3D12_VERTEXSHADER_H

#include "./VertexShader.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_VertexShader : public VertexShader
        {
        public:
            explicit D3D12_VertexShader(D3D12_Context* ctx);
            ~D3D12_VertexShader() override = default;

            void create() override;

            D3D12_SHADER_BYTECODE getShaderBytecode() const;

        private:
            D3D12_Context* m_ctx;
        };
    }
}

#endif // FCT_D3D12_VERTEXSHADER_H
