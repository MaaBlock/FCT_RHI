#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <span>
#include <cstdint>
#include <optional>
#include "../ThirdParty.h"
#include "./ShaderCompiler.h"

namespace FCT {

/**
 * @brief SPIR-V 转译配置参数
 */
struct ShaderCrossCompilerOptions {
    uint32_t glslVersion{300};                    // 目标 GLSL 版本，默认 300 (OpenGL ES 3.0 / WebGL 2.0)
    bool es{true};                                 // 启用 OpenGL ES 方言
    bool enable420Pack{false};                     // WebGL 2.0 强制禁用 420pack (消除 layout(binding=N))
    bool removeUnusedVariables{true};              // 剔除未引用的变量
    bool emitCombinedSamplers{true};               // 是否自动将分离式纹理与采样器合并为组合采样器
    std::string entryPoint{"main"};                // 入口点符号名
    ShaderKind stage{ShaderKind::VertexShader};    // 着色器管线阶段
    std::vector<std::pair<std::string, std::string>> macroDefinitions;
};

/**
 * @brief 组合采样器映射记录
 */
struct CombinedSamplerBinding {
    std::string combinedSamplerName;               // 合成后的组合采样器 uniform 符号名
    std::string textureName;                       // 原始独立纹理符号名
    std::string samplerName;                       // 原始独立采样器符号名
    uint32_t textureSlot{0};                       // 原始纹理绑定寄存器/槽位
    uint32_t samplerSlot{0};                       // 原始采样器绑定寄存器/槽位
};

/**
 * @brief 常量缓冲块 (UBO) 映射记录
 */
struct UniformBlockBinding {
    std::string blockName;                         // 常量块名称
    uint32_t slot{0};                              // 原始绑定位 (register bX)
    size_t sizeBytes{0};                           // 块数据结构体尺寸（字节）
};

/**
 * @brief 顶点输入属性位置映射记录
 */
struct VertexAttributeLocation {
    std::string attributeName;                     // 顶点属性名
    uint32_t location{0};                          // layout(location = N)
    uint32_t componentCount{0};                    // 向量维度 (1~4)
};

/**
 * @brief StorageBuffer (SSBO) 重写为 2D 数据纹理映射记录 (FEAT-30)
 */
struct StorageBufferBinding {
    std::string bufferName;                        // 原始 SSBO 变量/块符号名 (如 g_Instances 或 g_Buffer)
    std::string dataTextureName;                   // 重写后的 2D 数据纹理 uniform 符号名 (如 s_g_Instances)
    uint32_t slot{0};                              // 原始绑定槽位 (register tX)
};

/**
 * @brief 转译产物集合
 */
struct ShaderCrossCompilerResult {
    bool success{false};
    std::string glslSource;                        // 产出的 #version 300 es GLSL 源码
    std::vector<CombinedSamplerBinding> combinedSamplers;
    std::vector<UniformBlockBinding> uniformBlocks;
    std::vector<VertexAttributeLocation> vertexAttributes;
    std::vector<StorageBufferBinding> storageBuffers; // SSBO 重写映射表 (FEAT-30)
};

/**
 * @brief 基于 SPIRV-Cross 的着色器离线/运行时多目标转译器 (FEAT-30)
 */
class ShaderCrossCompiler {
public:
    ShaderCrossCompiler();
    ~ShaderCrossCompiler() = default;

    /**
     * @brief 纯 SPIR-V 字节码 -> #version 300 es GLSL 转译 (零 shaderc 依赖)
     * @param spirvWords SPIR-V 32 位整型指令流
     * @param options 转译控制选项
     * @return 成功返回结构化转译产物与反射表，失败返回 Status 错误
     */
    StatusOr<ShaderCrossCompilerResult> transpileSpirvToGlslEs300(
        std::span<const uint32_t> spirvWords,
        const ShaderCrossCompilerOptions& options);

    /**
     * @brief HLSL 源码 -> SPIR-V (通过 shaderc) -> #version 300 es GLSL (若 FCT_ENABLE_RUNTIME_SHADERC=0 则返回 UnimplementedError)
     */
    StatusOr<ShaderCrossCompilerResult> compileHlslToGlslEs300(
        std::string_view hlslSource,
        std::string_view entryPoint,
        ShaderKind stage,
        const ShaderCrossCompilerOptions& options);
};

} // namespace FCT
