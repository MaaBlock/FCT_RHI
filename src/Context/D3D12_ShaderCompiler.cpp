#include "../FCTAPI.h"
#include "./D3D12_ShaderCompiler.h"

namespace FCT
{
    std::string D3D12_ShaderCompiler::stripVulkanAttributes(const std::string& source) const
    {
        std::string result;
        result.reserve(source.size());

        size_t i = 0;
        const size_t len = source.size();
        while (i < len)
        {
            if (i + 1 < len && source[i] == '[' && source[i + 1] == '[')
            {
                size_t closePos = source.find("]]", i + 2);
                if (closePos != std::string::npos)
                {
                    i = closePos + 2;
                    continue;
                }
            }
            result.push_back(source[i]);
            ++i;
        }
        return result;
    }

    const char* D3D12_ShaderCompiler::getTargetProfile(ShaderKind kind) const
    {
        switch (kind)
        {
        case ShaderKind::VertexShader:
            return "vs_5_1";
        case ShaderKind::FragmentShader:
            return "ps_5_1";
        case ShaderKind::ComputeShader:
            return "cs_5_1";
        case ShaderKind::GeometryShader:
            return "gs_5_1";
        case ShaderKind::TessControlShader:
            return "hs_5_1";
        default:
            return "vs_5_1";
        }
    }

    std::vector<char> D3D12_ShaderCompiler::compile(const std::string& source, const std::string& entry_point, ShaderKind kind)
    {
        std::string cleanedSource = stripVulkanAttributes(source);
        const char* target = getTargetProfile(kind);

        UINT compileFlags = D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#ifdef FCT_DEBUG
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        Microsoft::WRL::ComPtr<ID3DBlob> byteCode;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3DCompile(
            cleanedSource.data(),
            cleanedSource.size(),
            "FCT_Shader",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entry_point.c_str(),
            target,
            compileFlags,
            0,
            &byteCode,
            &errorBlob);

        if (FAILED(hr))
        {
            if (errorBlob)
            {
                ferr << "D3DCompile failed: " << static_cast<const char*>(errorBlob->GetBufferPointer()) << std::endl;
            }
            else
            {
                ferr << "D3DCompile failed with HRESULT: " << std::hex << hr << std::dec << std::endl;
            }
            return {};
        }

        const char* bufferStart = static_cast<const char*>(byteCode->GetBufferPointer());
        size_t bufferSize = byteCode->GetBufferSize();
        return std::vector<char>(bufferStart, bufferStart + bufferSize);
    }
}
