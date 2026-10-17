#ifndef FCT_D3D12_SHADERBINARY_H
#define FCT_D3D12_SHADERBINARY_H

#include "./ShaderBinary.h"

namespace FCT
{
    class D3D12_Context;

    namespace RHI
    {
        class D3D12_ShaderBinary : public ShaderBinary
        {
        public:
            explicit D3D12_ShaderBinary(D3D12_Context* ctx);
            ~D3D12_ShaderBinary() = default;

            void create();

            D3D12_SHADER_BYTECODE getShaderBytecode() const;

        private:
            D3D12_Context* m_ctx;
        };
    }
}

#endif // FCT_D3D12_SHADERBINARY_H
