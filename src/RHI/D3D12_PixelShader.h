#ifndef FCT_D3D12_PIXELSHADER_H
#define FCT_D3D12_PIXELSHADER_H

#include "./PixelShader.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_PixelShader : public PixelShader
        {
        public:
            explicit D3D12_PixelShader(D3D12_Context* ctx);
            ~D3D12_PixelShader() override = default;

            void create() override;

            D3D12_SHADER_BYTECODE getShaderBytecode() const;

        private:
            D3D12_Context* m_ctx;
        };
    }
}

#endif // FCT_D3D12_PIXELSHADER_H
