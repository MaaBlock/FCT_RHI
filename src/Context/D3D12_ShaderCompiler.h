#ifndef FCT_D3D12_SHADERCOMPILER_H
#define FCT_D3D12_SHADERCOMPILER_H

#include "../ThirdParty.h"
#include "./ShaderCompiler.h"

namespace FCT
{
    class D3D12_ShaderCompiler : public ShaderCompiler
    {
    public:
        D3D12_ShaderCompiler() = default;
        ~D3D12_ShaderCompiler() override = default;

        std::vector<char> compile(const std::string& source, const std::string& entry_point, ShaderKind kind) override;

    private:
        std::string stripVulkanAttributes(const std::string& source) const;
        const char* getTargetProfile(ShaderKind kind) const;
    };
}

#endif // FCT_D3D12_SHADERCOMPILER_H
