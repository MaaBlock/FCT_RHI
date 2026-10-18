#ifndef FCT_D3D12_SHADERCOMPILER_H
#define FCT_D3D12_SHADERCOMPILER_H

#include "../ThirdParty.h"
#include "./ShaderCompiler.h"

namespace FCT
{
    class D3D12_ShaderCompiler : public ShaderCompiler
    {
    public:
        D3D12_ShaderCompiler();
        ~D3D12_ShaderCompiler() override = default;

        std::vector<char> compile(const std::string& source, const std::string& entry_point, ShaderKind kind) override;

    private:
        std::string translateViaSpirvCross(const std::string& source, const std::string& entry_point, ShaderKind kind, std::string& outEntryPoint);
        void extractVertexAttributeRemaps(const std::string& source, spirv_cross::CompilerHLSL& hlslCompiler) const;
        std::map<uint32_t, std::string> extractStageLocationTypes(const std::string& source, const char* structDecl) const;
        std::string padStageInterfaceStruct(const std::string& hlsl, const char* targetStructName, const std::map<uint32_t, std::string>& expectedLocations, bool isOutputStruct) const;
        std::string stripVulkanAttributes(const std::string& source) const;
        const char* getTargetProfile(ShaderKind kind) const;

        shaderc::Compiler m_shadercCompiler;
        shaderc::CompileOptions m_shadercOptions;
    };
}

#endif // FCT_D3D12_SHADERCOMPILER_H
