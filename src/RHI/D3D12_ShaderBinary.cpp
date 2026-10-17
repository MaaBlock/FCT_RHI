#include "../FCTAPI.h"
#include "./D3D12_ShaderBinary.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_ShaderBinary::D3D12_ShaderBinary(D3D12_Context* ctx)
            : m_ctx(ctx)
        {
        }

        void D3D12_ShaderBinary::create()
        {
        }

        D3D12_SHADER_BYTECODE D3D12_ShaderBinary::getShaderBytecode() const
        {
            D3D12_SHADER_BYTECODE bc{};
            bc.pShaderBytecode = m_code.empty() ? nullptr : m_code.data();
            bc.BytecodeLength = m_code.size();
            return bc;
        }
    }
}
