#include "../FCTAPI.h"
#include "./D3D12_ShaderCompiler.h"

namespace FCT
{
    D3D12_ShaderCompiler::D3D12_ShaderCompiler()
    {
        m_shadercOptions.SetSourceLanguage(shaderc_source_language_hlsl);
        m_shadercOptions.SetTargetSpirv(shaderc_spirv_version_1_5);
        m_shadercOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    }

    void D3D12_ShaderCompiler::extractVertexAttributeRemaps(const std::string& source, spirv_cross::CompilerHLSL& hlslCompiler) const
    {
        size_t structPos = source.find("struct ShaderIn");
        if (structPos == std::string::npos)
        {
            return;
        }
        size_t structEnd = source.find("};", structPos);
        if (structEnd == std::string::npos)
        {
            return;
        }

        std::string_view block(source.data() + structPos, structEnd - structPos);
        size_t pos = 0;
        const std::string_view locTag = "[[vk::location(";
        while ((pos = block.find(locTag, pos)) != std::string_view::npos)
        {
            pos += locTag.size();
            size_t closeParen = block.find(")]]", pos);
            if (closeParen == std::string_view::npos)
            {
                break;
            }
            uint32_t location = static_cast<uint32_t>(std::stoul(std::string(block.substr(pos, closeParen - pos))));
            size_t lineEnd = block.find(';', closeParen);
            if (lineEnd == std::string_view::npos)
            {
                break;
            }
            size_t colonPos = block.rfind(':', lineEnd);
            if (colonPos != std::string_view::npos && colonPos > closeParen)
            {
                std::string_view semView = block.substr(colonPos + 1, lineEnd - (colonPos + 1));
                while (!semView.empty() && (semView.front() == ' ' || semView.front() == '\t'))
                {
                    semView.remove_prefix(1);
                }
                while (!semView.empty() && (semView.back() == ' ' || semView.back() == '\t'))
                {
                    semView.remove_suffix(1);
                }
                if (!semView.empty())
                {
                    hlslCompiler.add_vertex_attribute_remap({ location, std::string(semView) });
                }
            }
            pos = lineEnd + 1;
        }
    }

    std::map<uint32_t, std::string> D3D12_ShaderCompiler::extractStageLocationTypes(const std::string& source, const char* structDecl) const
    {
        std::map<uint32_t, std::string> result;
        size_t structPos = source.find(structDecl);
        if (structPos == std::string::npos)
        {
            return result;
        }
        size_t structEnd = source.find("};", structPos);
        if (structEnd == std::string::npos)
        {
            return result;
        }

        std::string_view block(source.data() + structPos, structEnd - structPos);
        size_t pos = 0;
        const std::string_view locTag = "[[vk::location(";
        while ((pos = block.find(locTag, pos)) != std::string_view::npos)
        {
            pos += locTag.size();
            size_t closeParen = block.find(")]]", pos);
            if (closeParen == std::string_view::npos)
            {
                break;
            }
            uint32_t location = static_cast<uint32_t>(std::stoul(std::string(block.substr(pos, closeParen - pos))));
            size_t afterAttr = closeParen + 3;
            size_t lineEnd = block.find(';', afterAttr);
            if (lineEnd == std::string_view::npos)
            {
                break;
            }
            std::string_view declView = block.substr(afterAttr, lineEnd - afterAttr);
            while (!declView.empty() && (declView.front() == ' ' || declView.front() == '\t'))
            {
                declView.remove_prefix(1);
            }
            size_t spacePos = declView.find_first_of(" \t");
            if (spacePos != std::string_view::npos)
            {
                result[location] = std::string(declView.substr(0, spacePos));
            }
            pos = lineEnd + 1;
        }
        return result;
    }

    std::string D3D12_ShaderCompiler::padStageInterfaceStruct(
        const std::string& hlsl,
        const char* targetStructName,
        const std::map<uint32_t, std::string>& expectedLocations,
        bool isOutputStruct) const
    {
        if (expectedLocations.empty())
        {
            return hlsl;
        }

        size_t structPos = hlsl.find(targetStructName);
        if (structPos == std::string::npos)
        {
            return hlsl;
        }
        size_t openBrace = hlsl.find('{', structPos);
        if (openBrace == std::string::npos)
        {
            return hlsl;
        }
        size_t closeBrace = hlsl.find("};", openBrace);
        if (closeBrace == std::string::npos)
        {
            return hlsl;
        }

        std::map<uint32_t, std::string> existingTexcoords;
        std::vector<std::string> otherMembers;

        std::string body = hlsl.substr(openBrace + 1, closeBrace - (openBrace + 1));
        std::istringstream iss(body);
        std::string line;
        const std::string texcoordTag = ": TEXCOORD";
        while (std::getline(iss, line))
        {
            size_t firstNonSpace = line.find_first_not_of(" \t\r\n");
            if (firstNonSpace == std::string::npos)
            {
                continue;
            }
            size_t lastNonSpace = line.find_last_not_of(" \t\r\n");
            std::string trimmed = line.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);

            size_t tagPos = trimmed.find(texcoordTag);
            if (tagPos != std::string::npos)
            {
                size_t numStart = tagPos + texcoordTag.size();
                size_t semiPos = trimmed.find(';', numStart);
                if (semiPos != std::string::npos && semiPos > numStart)
                {
                    uint32_t loc = static_cast<uint32_t>(std::stoul(trimmed.substr(numStart, semiPos - numStart)));
                    existingTexcoords[loc] = trimmed;
                    continue;
                }
            }
            otherMembers.push_back(trimmed);
        }

        std::ostringstream newBody;
        newBody << "\n";
        for (const auto& [loc, typeStr] : expectedLocations)
        {
            auto it = existingTexcoords.find(loc);
            if (it != existingTexcoords.end())
            {
                newBody << "    " << it->second << "\n";
            }
            else
            {
                newBody << "    " << typeStr << " _fct_pad_loc" << loc << " : TEXCOORD" << loc << ";\n";
            }
        }
        for (const auto& [loc, decl] : existingTexcoords)
        {
            if (expectedLocations.find(loc) == expectedLocations.end())
            {
                newBody << "    " << decl << "\n";
            }
        }
        for (const auto& other : otherMembers)
        {
            newBody << "    " << other << "\n";
        }

        std::string patched = hlsl.substr(0, openBrace + 1) + newBody.str() + hlsl.substr(closeBrace);
        if (isOutputStruct)
        {
            const std::string uninitDecl = "SPIRV_Cross_Output stage_output;";
            size_t declPos = patched.find(uninitDecl);
            if (declPos != std::string::npos)
            {
                patched.replace(declPos, uninitDecl.size(), "SPIRV_Cross_Output stage_output = (SPIRV_Cross_Output)0;");
            }
        }
        return patched;
    }

    std::string D3D12_ShaderCompiler::translateViaSpirvCross(
        const std::string& source,
        const std::string& entry_point,
        ShaderKind kind,
        std::string& outEntryPoint)
    {
        auto spvResult = m_shadercCompiler.CompileGlslToSpv(
            source.c_str(),
            source.size(),
            FCTShaderKindToShaderrcShaderKind(kind),
            "FCTShader",
            entry_point.c_str(),
            m_shadercOptions);

        if (spvResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            ferr << "Shaderc HLSL->SPIR-V failed: " << spvResult.GetErrorMessage() << std::endl;
            outEntryPoint = entry_point;
            return stripVulkanAttributes(source);
        }

        std::vector<uint32_t> spv(spvResult.cbegin(), spvResult.cend());
        try
        {
            spirv_cross::CompilerHLSL hlslCompiler(std::move(spv));
            auto hlslOpts = hlslCompiler.get_hlsl_options();
            hlslOpts.shader_model = 51;
            hlslOpts.use_entry_point_name = true;
            hlslCompiler.set_hlsl_options(hlslOpts);

            if (kind == ShaderKind::VertexShader)
            {
                extractVertexAttributeRemaps(source, hlslCompiler);
            }

            outEntryPoint = entry_point;
            std::string compiled = hlslCompiler.compile();
            if (kind == ShaderKind::VertexShader)
            {
                auto expectedOut = extractStageLocationTypes(source, "struct ShaderOut");
                compiled = padStageInterfaceStruct(compiled, "struct SPIRV_Cross_Output", expectedOut, true);
            }
            else if (kind == ShaderKind::FragmentShader)
            {
                auto expectedIn = extractStageLocationTypes(source, "struct ShaderIn");
                compiled = padStageInterfaceStruct(compiled, "struct SPIRV_Cross_Input", expectedIn, false);
            }
            return compiled;
        }
        catch (const std::exception& e)
        {
            ferr << "SPIRV-Cross HLSL translation failed: " << e.what() << std::endl;
            outEntryPoint = entry_point;
            return stripVulkanAttributes(source);
        }
    }

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
        ZoneScoped();
        std::string finalEntryPoint = entry_point;
        std::string hlslSource = translateViaSpirvCross(source, entry_point, kind, finalEntryPoint);
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
            hlslSource.data(),
            hlslSource.size(),
            "FCT_Shader",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            finalEntryPoint.c_str(),
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
