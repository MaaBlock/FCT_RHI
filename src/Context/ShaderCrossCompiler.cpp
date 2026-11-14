#include "./ShaderCrossCompiler.h"
#include <spirv_cross/spirv_cross_c.h>
#include <cctype>

#if defined(FCT_ENABLE_RUNTIME_SHADERC) && FCT_ENABLE_RUNTIME_SHADERC
#include <shaderc/shaderc.hpp>
#include "./Shaderc_ShaderCompiler.h"
#endif

namespace FCT {

namespace {

struct SpvcContextGuard {
    spvc_context ctx{nullptr};

    SpvcContextGuard() = default;
    ~SpvcContextGuard() {
        if (ctx != nullptr) {
            spvc_context_destroy(ctx);
        }
    }

    SpvcContextGuard(const SpvcContextGuard&) = delete;
    SpvcContextGuard& operator=(const SpvcContextGuard&) = delete;
};

std::string safeSpvcString(const char* str) {
    return (str != nullptr) ? std::string(str) : std::string();
}

Status makeSpvcStatusError(spvc_context ctx, const char* stepDesc) {
    const char* detail = (ctx != nullptr) ? spvc_context_get_last_error_string(ctx) : nullptr;
    std::string msg = std::string("ShaderCrossCompiler SPIRV-Cross ") + stepDesc;
    if (detail != nullptr && detail[0] != '\0') {
        msg += ": ";
        msg += detail;
    }
    return InternalError(msg);
}

Status configureGlslCompilerOptions(
    spvc_context ctx,
    spvc_compiler compiler,
    const ShaderCrossCompilerOptions& options)
{
    spvc_compiler_options spvcOpts = nullptr;
    if (spvc_compiler_create_compiler_options(compiler, &spvcOpts) != SPVC_SUCCESS || spvcOpts == nullptr) {
        return makeSpvcStatusError(ctx, "创建编译选项失败");
    }

    const spvc_bool esFlag = options.es ? SPVC_TRUE : SPVC_FALSE;
    const spvc_bool pack420Flag = options.enable420Pack ? SPVC_TRUE : SPVC_FALSE;

    if (spvc_compiler_options_set_uint(spvcOpts, SPVC_COMPILER_OPTION_GLSL_VERSION, options.glslVersion) != SPVC_SUCCESS ||
        spvc_compiler_options_set_bool(spvcOpts, SPVC_COMPILER_OPTION_GLSL_ES, esFlag) != SPVC_SUCCESS ||
        spvc_compiler_options_set_bool(spvcOpts, SPVC_COMPILER_OPTION_GLSL_ENABLE_420PACK_EXTENSION, pack420Flag) != SPVC_SUCCESS ||
        spvc_compiler_options_set_bool(spvcOpts, SPVC_COMPILER_OPTION_GLSL_VULKAN_SEMANTICS, SPVC_FALSE) != SPVC_SUCCESS ||
        spvc_compiler_options_set_bool(spvcOpts, SPVC_COMPILER_OPTION_GLSL_SEPARATE_SHADER_OBJECTS, SPVC_FALSE) != SPVC_SUCCESS ||
        spvc_compiler_options_set_bool(spvcOpts, SPVC_COMPILER_OPTION_GLSL_EMIT_PUSH_CONSTANT_AS_UNIFORM_BUFFER, SPVC_FALSE) != SPVC_SUCCESS) {
        return makeSpvcStatusError(ctx, "设置 GLSL ES 编译选项失败");
    }

    if (spvc_compiler_install_compiler_options(compiler, spvcOpts) != SPVC_SUCCESS) {
        return makeSpvcStatusError(ctx, "安装 GLSL ES 编译选项失败");
    }
    return OkStatus();
}

Status applyCombinedSamplerRemapping(spvc_context ctx, spvc_compiler compiler)
{
    spvc_variable_id dummySamplerId = 0;
    if (spvc_compiler_build_dummy_sampler_for_combined_images(compiler, &dummySamplerId) != SPVC_SUCCESS) {
        return makeSpvcStatusError(ctx, "构建哑采样器失败");
    }
    if (spvc_compiler_build_combined_image_samplers(compiler) != SPVC_SUCCESS) {
        return makeSpvcStatusError(ctx, "构建组合图像采样器失败");
    }

    const spvc_combined_image_sampler* samplers = nullptr;
    size_t numSamplers = 0;
    if (spvc_compiler_get_combined_image_samplers(compiler, &samplers, &numSamplers) != SPVC_SUCCESS) {
        return makeSpvcStatusError(ctx, "枚举组合图像采样器失败");
    }

    for (size_t i = 0; i < numSamplers; ++i) {
        const spvc_combined_image_sampler& remap = samplers[i];

        // 等价继承分离式纹理的 DescriptorSet 与 Binding 装饰符
        if (spvc_compiler_has_decoration(compiler, remap.image_id, SpvDecorationDescriptorSet) == SPVC_TRUE) {
            const unsigned setIdx = spvc_compiler_get_decoration(compiler, remap.image_id, SpvDecorationDescriptorSet);
            spvc_compiler_set_decoration(compiler, remap.combined_id, SpvDecorationDescriptorSet, setIdx);
        }
        if (spvc_compiler_has_decoration(compiler, remap.image_id, SpvDecorationBinding) == SPVC_TRUE) {
            const unsigned bindingIdx = spvc_compiler_get_decoration(compiler, remap.image_id, SpvDecorationBinding);
            spvc_compiler_set_decoration(compiler, remap.combined_id, SpvDecorationBinding, bindingIdx);
        }

        // 赋予确定性组合采样器 uniform 符号名 (FEAT-30)
        std::string texName = safeSpvcString(spvc_compiler_get_name(compiler, remap.image_id));
        std::string smpName = safeSpvcString(spvc_compiler_get_name(compiler, remap.sampler_id));
        if (texName.empty()) {
            texName = "tex" + std::to_string(remap.image_id);
        }
        if (smpName.empty()) {
            smpName = "smp" + std::to_string(remap.sampler_id);
        }
        const std::string combinedName = "SPIRV_Cross_Combined" + texName + smpName;
        spvc_compiler_set_name(compiler, remap.combined_id, combinedName.c_str());
    }

    return OkStatus();
}

void eraseBufferBlockDeclaration(std::string& glslOutput, const std::string& blockName)
{
    const std::string bufferDeclPrefix = "buffer " + blockName;
    const size_t bufPos = glslOutput.find(bufferDeclPrefix);
    if (bufPos == std::string::npos) {
        return;
    }

    size_t layoutStart = glslOutput.rfind("layout", bufPos);
    if (layoutStart == std::string::npos ||
        (glslOutput.find(';', layoutStart) != std::string::npos && glslOutput.find(';', layoutStart) < bufPos)) {
        layoutStart = bufPos;
    }

    const size_t braceOpen = glslOutput.find('{', bufPos);
    if (braceOpen == std::string::npos) {
        return;
    }

    int depth = 1;
    size_t braceClose = braceOpen + 1;
    while (braceClose < glslOutput.length() && depth > 0) {
        if (glslOutput[braceClose] == '{') {
            depth++;
        } else if (glslOutput[braceClose] == '}') {
            depth--;
        }
        if (depth == 0) {
            break;
        }
        braceClose++;
    }

    if (depth == 0) {
        const size_t semiPos = glslOutput.find(';', braceClose);
        if (semiPos != std::string::npos) {
            glslOutput.erase(layoutStart, semiPos - layoutStart + 1);
        }
    }
}

void replaceBufferDataAccessExpressions(
    std::string& glslOutput,
    const std::string& instName,
    const std::string& varName,
    const std::string& blockName,
    const std::string& funcName)
{
    size_t dotPos = 0;
    while ((dotPos = glslOutput.find("._data[", dotPos)) != std::string::npos) {
        const size_t identEnd = dotPos;
        size_t identStart = identEnd;
        while (identStart > 0 &&
               (std::isalnum(static_cast<unsigned char>(glslOutput[identStart - 1])) ||
                glslOutput[identStart - 1] == '_')) {
            identStart--;
        }

        const std::string accessIdent = glslOutput.substr(identStart, identEnd - identStart);
        const bool match =
            (accessIdent == instName || accessIdent == varName || accessIdent == blockName ||
             (!instName.empty() && accessIdent.find(instName) != std::string::npos) ||
             (!varName.empty() && accessIdent.find(varName) != std::string::npos));

        if (match) {
            const size_t openBracket = dotPos + 6;
            int depth = 1;
            size_t closeBracket = openBracket + 1;
            while (closeBracket < glslOutput.length() && depth > 0) {
                if (glslOutput[closeBracket] == '[' || glslOutput[closeBracket] == '(') {
                    depth++;
                } else if (glslOutput[closeBracket] == ']' || glslOutput[closeBracket] == ')') {
                    depth--;
                }
                if (depth == 0) {
                    break;
                }
                closeBracket++;
            }
            if (depth == 0 && closeBracket < glslOutput.length()) {
                const std::string indexExpr = glslOutput.substr(openBracket + 1, closeBracket - openBracket - 1);
                const std::string call = funcName + "(" + indexExpr + ")";
                glslOutput.replace(identStart, closeBracket - identStart + 1, call);
                dotPos = identStart + call.length();
                continue;
            }
        }
        dotPos += 7;
    }
}

void insertDataTextureHelpersAfterHeader(std::string& glslOutput, const std::string& helpers)
{
    const size_t insertPos = glslOutput.find("#version 300 es");
    if (insertPos == std::string::npos) {
        glslOutput = helpers + glslOutput;
        return;
    }

    size_t lineEnd = glslOutput.find('\n', insertPos);
    while (lineEnd != std::string::npos) {
        size_t nextLine = lineEnd + 1;
        while (nextLine < glslOutput.size() &&
               (glslOutput[nextLine] == ' ' || glslOutput[nextLine] == '\t' ||
                glslOutput[nextLine] == '\r' || glslOutput[nextLine] == '\n')) {
            nextLine++;
        }
        if (glslOutput.compare(nextLine, 9, "precision") == 0) {
            lineEnd = glslOutput.find('\n', nextLine);
        } else {
            break;
        }
    }

    if (lineEnd != std::string::npos) {
        glslOutput.insert(lineEnd + 1, helpers);
    } else {
        glslOutput += helpers;
    }
}

Status rewriteStorageBuffersToDataTextures(
    spvc_context ctx,
    spvc_compiler compiler,
    spvc_resources resources,
    std::string& glslOutput,
    std::vector<StorageBufferBinding>& outStorageBuffers)
{
    const spvc_reflected_resource* ssboList = nullptr;
    size_t numSsbos = 0;
    if (spvc_resources_get_resource_list_for_type(
            resources, SPVC_RESOURCE_TYPE_STORAGE_BUFFER, &ssboList, &numSsbos) != SPVC_SUCCESS) {
        return makeSpvcStatusError(ctx, "反射 StorageBuffer 列表失败");
    }

    if (numSsbos == 0) {
        return OkStatus();
    }

    std::string helpers;
    helpers += "\n// --- WebGL 2.0 2D 数据纹理 StorageBuffer 模拟解包辅助库 (FEAT-30) ---\n";
    helpers += "#ifndef FCT_TEXEL_FETCH_2D_HELPER\n";
    helpers += "#define FCT_TEXEL_FETCH_2D_HELPER\n";
    helpers += "ivec2 fct_calc_texel_coord(uint texelIndex) {\n";
    helpers += "    return ivec2(int(texelIndex & 1023u), int(texelIndex >> 10u));\n";
    helpers += "}\n";
    helpers += "#endif\n\n";

    for (size_t i = 0; i < numSsbos; ++i) {
        const spvc_reflected_resource& ssbo = ssboList[i];
        std::string varName = safeSpvcString(spvc_compiler_get_name(compiler, ssbo.id));
        const std::string blockName = safeSpvcString(spvc_compiler_get_name(compiler, ssbo.base_type_id));
        if (varName.empty()) {
            varName = blockName;
        }
        if (varName.empty()) {
            varName = "storageBuffer_" + std::to_string(ssbo.id);
        }
        const uint32_t slot = spvc_compiler_get_decoration(compiler, ssbo.id, SpvDecorationBinding);

        const std::string dataTexName = "s_" + varName;
        outStorageBuffers.push_back({varName, dataTexName, slot});

        spvc_type structType = spvc_compiler_get_type_handle(compiler, ssbo.base_type_id);
        std::string memberTypeName = "vec4";
        bool isCustomStruct = false;
        spvc_type elemType = nullptr;
        spvc_type_id elemSelfId = 0;

        if (structType != nullptr && spvc_type_get_num_member_types(structType) > 0) {
            const spvc_type_id elemTypeId = spvc_type_get_member_type(structType, 0);
            elemType = spvc_compiler_get_type_handle(compiler, elemTypeId);
            if (elemType != nullptr) {
                elemSelfId = spvc_type_get_base_type_id(elemType);
                const spvc_basetype baseType = spvc_type_get_basetype(elemType);
                const unsigned vecSize = spvc_type_get_vector_size(elemType);
                if (baseType == SPVC_BASETYPE_STRUCT) {
                    memberTypeName = safeSpvcString(spvc_compiler_get_name(compiler, elemSelfId));
                    isCustomStruct = true;
                } else if (baseType == SPVC_BASETYPE_UINT32) {
                    memberTypeName = (vecSize == 4) ? "uvec4" : "uint";
                } else if (baseType == SPVC_BASETYPE_INT32) {
                    memberTypeName = (vecSize == 4) ? "ivec4" : "int";
                } else {
                    memberTypeName = (vecSize == 4) ? "vec4" : "float";
                }
            }
        }

        helpers += "uniform highp usampler2D " + dataTexName + ";\n";
        const std::string funcName = "fetch_" + varName;

        if (!isCustomStruct || elemType == nullptr) {
            helpers += memberTypeName + " " + funcName + "(uint idx) {\n";
            helpers += "    uvec4 raw = texelFetch(" + dataTexName + ", fct_calc_texel_coord(idx), 0);\n";
            if (memberTypeName == "vec4") {
                helpers += "    return uintBitsToFloat(raw);\n";
            } else if (memberTypeName == "float") {
                helpers += "    return uintBitsToFloat(raw.x);\n";
            } else if (memberTypeName == "uvec4") {
                helpers += "    return raw;\n";
            } else if (memberTypeName == "uint") {
                helpers += "    return raw.x;\n";
            } else if (memberTypeName == "ivec4") {
                helpers += "    return ivec4(raw);\n";
            } else {
                helpers += "    return int(raw.x);\n";
            }
            helpers += "}\n";
            helpers += memberTypeName + " " + funcName + "(int idx) { return " + funcName + "(uint(idx)); }\n\n";
        } else {
            const unsigned numMembers = spvc_type_get_num_member_types(elemType);
            helpers += memberTypeName + " " + funcName + "(uint idx) {\n";
            helpers += "    " + memberTypeName + " res;\n";
            for (unsigned m = 0; m < numMembers; ++m) {
                std::string memName = safeSpvcString(spvc_compiler_get_member_name(compiler, elemSelfId, m));
                if (memName.empty()) {
                    memName = "m" + std::to_string(m);
                }
                helpers += "    res." + memName + " = uintBitsToFloat(texelFetch(" + dataTexName +
                           ", fct_calc_texel_coord(idx * " + std::to_string(numMembers) + "u + " +
                           std::to_string(m) + "u), 0));\n";
            }
            helpers += "    return res;\n";
            helpers += "}\n";
            helpers += memberTypeName + " " + funcName + "(int idx) { return " + funcName + "(uint(idx)); }\n\n";
        }

        eraseBufferBlockDeclaration(glslOutput, blockName);

        const std::string instName = safeSpvcString(spvc_compiler_get_name(compiler, ssbo.id));
        replaceBufferDataAccessExpressions(glslOutput, instName, varName, blockName, funcName);
    }

    insertDataTextureHelpersAfterHeader(glslOutput, helpers);
    return OkStatus();
}

Status extractReflectionTables(
    spvc_context ctx,
    spvc_compiler compiler,
    spvc_resources resources,
    ShaderCrossCompilerResult& outResult)
{
    const spvc_combined_image_sampler* samplers = nullptr;
    size_t numSamplers = 0;
    if (spvc_compiler_get_combined_image_samplers(compiler, &samplers, &numSamplers) != SPVC_SUCCESS) {
        return makeSpvcStatusError(ctx, "读取组合采样器反射表失败");
    }
    for (size_t i = 0; i < numSamplers; ++i) {
        const spvc_combined_image_sampler& remap = samplers[i];
        CombinedSamplerBinding binding;
        binding.combinedSamplerName = safeSpvcString(spvc_compiler_get_name(compiler, remap.combined_id));
        binding.textureName = safeSpvcString(spvc_compiler_get_name(compiler, remap.image_id));
        binding.samplerName = safeSpvcString(spvc_compiler_get_name(compiler, remap.sampler_id));
        binding.textureSlot = spvc_compiler_get_decoration(compiler, remap.image_id, SpvDecorationBinding);
        binding.samplerSlot = spvc_compiler_get_decoration(compiler, remap.sampler_id, SpvDecorationBinding);
        outResult.combinedSamplers.push_back(std::move(binding));
    }

    const spvc_reflected_resource* uboList = nullptr;
    size_t numUbos = 0;
    if (spvc_resources_get_resource_list_for_type(
            resources, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, &uboList, &numUbos) != SPVC_SUCCESS) {
        return makeSpvcStatusError(ctx, "读取 UniformBuffer 反射表失败");
    }
    for (size_t i = 0; i < numUbos; ++i) {
        const spvc_reflected_resource& ubo = uboList[i];
        UniformBlockBinding uboBinding;
        uboBinding.blockName = safeSpvcString(spvc_compiler_get_name(compiler, ubo.id));
        uboBinding.slot = spvc_compiler_get_decoration(compiler, ubo.id, SpvDecorationBinding);
        spvc_type uboType = spvc_compiler_get_type_handle(compiler, ubo.type_id);
        size_t declaredSize = 0;
        if (uboType != nullptr &&
            spvc_compiler_get_declared_struct_size(compiler, uboType, &declaredSize) == SPVC_SUCCESS) {
            uboBinding.sizeBytes = declaredSize;
        }
        outResult.uniformBlocks.push_back(std::move(uboBinding));
    }

    const spvc_reflected_resource* inputList = nullptr;
    size_t numInputs = 0;
    if (spvc_resources_get_resource_list_for_type(
            resources, SPVC_RESOURCE_TYPE_STAGE_INPUT, &inputList, &numInputs) != SPVC_SUCCESS) {
        return makeSpvcStatusError(ctx, "读取顶点输入属性反射表失败");
    }
    for (size_t i = 0; i < numInputs; ++i) {
        const spvc_reflected_resource& input = inputList[i];
        VertexAttributeLocation attr;
        attr.attributeName = safeSpvcString(spvc_compiler_get_name(compiler, input.id));
        attr.location = spvc_compiler_get_decoration(compiler, input.id, SpvDecorationLocation);
        spvc_type inputType = spvc_compiler_get_type_handle(compiler, input.type_id);
        attr.componentCount = (inputType != nullptr) ? spvc_type_get_vector_size(inputType) : 0u;
        outResult.vertexAttributes.push_back(std::move(attr));
    }

    return OkStatus();
}

} // namespace

ShaderCrossCompiler::ShaderCrossCompiler() = default;

StatusOr<ShaderCrossCompilerResult> ShaderCrossCompiler::transpileSpirvToGlslEs300(
    std::span<const uint32_t> spirvWords,
    const ShaderCrossCompilerOptions& options)
{
    if (spirvWords.empty()) {
        return InvalidArgumentError("ShaderCrossCompiler: SPIR-V 字节码为空。");
    }

    SpvcContextGuard ctxGuard;
    if (spvc_context_create(&ctxGuard.ctx) != SPVC_SUCCESS || ctxGuard.ctx == nullptr) {
        return InternalError("ShaderCrossCompiler: 无法创建 SPIRV-Cross C 上下文。");
    }

    spvc_parsed_ir parsedIr = nullptr;
    if (spvc_context_parse_spirv(ctxGuard.ctx, spirvWords.data(), spirvWords.size(), &parsedIr) != SPVC_SUCCESS ||
        parsedIr == nullptr) {
        return makeSpvcStatusError(ctxGuard.ctx, "SPIR-V 字节码解析失败");
    }

    spvc_compiler compiler = nullptr;
    if (spvc_context_create_compiler(
            ctxGuard.ctx,
            SPVC_BACKEND_GLSL,
            parsedIr,
            SPVC_CAPTURE_MODE_TAKE_OWNERSHIP,
            &compiler) != SPVC_SUCCESS ||
        compiler == nullptr) {
        return makeSpvcStatusError(ctxGuard.ctx, "创建 GLSL 后端编译器失败");
    }

    Status optStatus = configureGlslCompilerOptions(ctxGuard.ctx, compiler, options);
    if (!optStatus.ok()) {
        return optStatus;
    }

    if (options.emitCombinedSamplers) {
        Status remapStatus = applyCombinedSamplerRemapping(ctxGuard.ctx, compiler);
        if (!remapStatus.ok()) {
            return remapStatus;
        }
    }

    const char* compiledGlsl = nullptr;
    if (spvc_compiler_compile(compiler, &compiledGlsl) != SPVC_SUCCESS || compiledGlsl == nullptr) {
        return makeSpvcStatusError(ctxGuard.ctx, "GLSL ES 源码生成失败");
    }

    std::string glslOutput(compiledGlsl);

    spvc_resources resources = nullptr;
    if (spvc_compiler_create_shader_resources(compiler, &resources) != SPVC_SUCCESS || resources == nullptr) {
        return makeSpvcStatusError(ctxGuard.ctx, "着色器资源反射构建失败");
    }

    ShaderCrossCompilerResult result;
    result.success = true;

    Status ssboStatus = rewriteStorageBuffersToDataTextures(
        ctxGuard.ctx, compiler, resources, glslOutput, result.storageBuffers);
    if (!ssboStatus.ok()) {
        return ssboStatus;
    }

    result.glslSource = std::move(glslOutput);

    Status reflectStatus = extractReflectionTables(ctxGuard.ctx, compiler, resources, result);
    if (!reflectStatus.ok()) {
        return reflectStatus;
    }

    return result;
}

StatusOr<ShaderCrossCompilerResult> ShaderCrossCompiler::compileHlslToGlslEs300(
    std::string_view hlslSource,
    std::string_view entryPoint,
    ShaderKind stage,
    const ShaderCrossCompilerOptions& options)
{
#if defined(FCT_ENABLE_RUNTIME_SHADERC) && FCT_ENABLE_RUNTIME_SHADERC
    if (hlslSource.empty()) {
        return InvalidArgumentError("ShaderCrossCompiler: HLSL 源代码为空。");
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions shadercOpts;
    shadercOpts.SetSourceLanguage(shaderc_source_language_hlsl);
    shadercOpts.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
    shadercOpts.AddMacroDefinition("FCT_TARGET_WGL2", "1");
    shadercOpts.AddMacroDefinition("FCT_TARGET_GLES3", "1");
    shadercOpts.AddMacroDefinition("FCT_DATA_TEX_WIDTH", "1024");

    for (const auto& [name, val] : options.macroDefinitions) {
        shadercOpts.AddMacroDefinition(name, val);
    }

    const shaderc_shader_kind kind = FCTShaderKindToShaderrcShaderKind(stage);
    const std::string ep(entryPoint.empty() ? "main" : entryPoint);

    auto module = compiler.CompileGlslToSpv(
        hlslSource.data(),
        hlslSource.size(),
        kind,
        "source.hlsl",
        ep.c_str(),
        shadercOpts);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        return InternalError("ShaderCrossCompiler shaderc 编译错误: " + module.GetErrorMessage());
    }

    const std::span<const uint32_t> spirvWords(module.cbegin(), module.cend() - module.cbegin());
    return transpileSpirvToGlslEs300(spirvWords, options);
#else
    (void)hlslSource;
    (void)entryPoint;
    (void)stage;
    (void)options;
    return UnimplementedError("ShaderCrossCompiler: 运行时 shaderc 编译已被剥离 (FCT_ENABLE_RUNTIME_SHADERC=0)。");
#endif
}

} // namespace FCT
