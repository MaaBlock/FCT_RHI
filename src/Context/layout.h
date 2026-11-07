#ifndef LAYOUT_H
#define LAYOUT_H
#include "ShaderStage.h"
#include "Context.h"
#include "UniformWrapper.h"
#include "VertexShader.h"

namespace FCT {
    // --- 前向声明 ---
    class DepthStencilState;
    class Image;
    class Sampler;
    class PassResource;
    namespace RHI {
        class Pass;
        class CommandBuffer;
        class RasterizationPipeline;
    }

    // --- 用于构造 Layout 的参数结构体 ---
    /**
     * @brief 定义一个纹理槽位，用于在着色器中绑定纹理。
     */
    struct TextureSlot
    {
        std::string name;
        FCT::TextureType type = FCT::TextureType::Texture2D;
    };

    /**
     * @brief 定义 Layout 将要附加到的渲染通道(Render Pass)的名称。
     */
    struct PassName
    {
        std::string name;
    };

    /**
     * @brief 定义着色器代码的引用，包含代码本身、哈希值和类型。
     */
    struct ShaderRef
    {
        std::string code;
        size_t hash;
        ShaderKind kind;
    };

    // --- 类型别名 ---
    using UniformSlot = FCT::ConstLayout;
    using UniformVar = FCT::ConstElement;
    using UniformType = FCT::ConstType;
    using SamplerSlot = FCT::SamplerElement;

    /**
     * @class Layout
     * @brief 管理渲染管线的状态、资源布局和缓存。
     * @details
     * Layout 是 FCT 库的核心类之一，它封装了与特定渲染效果相关的所有配置，
     * 包括顶点布局、资源（纹理、常量缓冲区、采样器）布局、着色器以及其他管线状态。
     * 它通过构造函数链式配置，并提供了一套完整的渲染命令接口。
     */
    class Layout {
    public:
        struct DisableInjectPassResource
        {

        };
    public:
        /** @name 构造与上下文设置
         *  @{
        */
        template<typename... Args>
        Layout(FCT::Context* ctx,Args... args);
        ~Layout();
        /**
         * @cond CHINESE
         * @brief 重新设置 FCT 上下文。
         * @param ctx 新的 FCT 上下文指针。
         * @endcond
         */
        void ctx(FCT::Context* ctx);
        /** @} */

        /** @name 动态布局配置
         *  @{
         */

        /**
         * @cond CHINESE
         * @brief 添加一个顶点布局。
         * @param vertexLayout 顶点布局定义。
         * @note 修改布局会清空管线和资源缓存。
         * @endcond
         */
        void addVertexLayout(const FCT::VertexLayout& vertexLayout);

        /**
         * @cond CHINESE
         * @brief 设置像素（片元）着色器的输出布局。
         * @param pixelLayout 像素布局定义。
         * @note 修改布局会清空管线和资源缓存。
         * @endcond
         */
        void setPixelLayout(const FCT::PixelLayout& pixelLayout);

        /**
         * @cond CHINESE
         * @brief 添加一个常量缓冲区（Uniform Buffer）的布局槽位。
         * @param uniformSlot 常量缓冲区的布局定义。
         * @note 修改布局会清空管线和资源缓存。
         * @endcond
         */
        void addUniformSlot(const UniformSlot& uniformSlot);

        void addTextureSlot(const FCT::TextureSlot& element);
        /**
         * @cond CHINESE
         * @brief 添加一个纹理资源槽位。
         * @param element 纹理元素定义。
         * @note 修改布局会清空管线和资源缓存。
         * @endcond
         */
        void addTextureSlot(FCT::TextureElement element);

        /**
         * @cond CHINESE
         * @brief 添加一个采样器资源槽位。
         * @param samplerSlot 采样器元素定义。
         * @note 修改布局会清空管线和资源缓存。
         * @endcond
         */
        void addSamplerSlot(const SamplerSlot& samplerSlot);

        /**
         * @cond CHINESE
         * @brief 移除一个纹理资源槽位。
         * @param name 要移除的纹理槽位名称。
         * @note 修改布局会清空管线和资源缓存。
         * @endcond
         */
        void removeTextureSlot(const char* name);

        /**
         * @cond CHINESE
         * @brief 设置一个固定的图像资源，该资源不会在 begin/end 之间被清除。
         * @param name 资源在着色器中的名称。
         * @param image 指向 FCT::Image 对象的指针。
         * @endcond
         */
        void setFixedImage(std::string name, FCT::Image* image);

        /**
         * @cond CHINESE
         * @brief 将此 Layout 附加到一个渲染通道。
         * @param passName 渲染通道的名称。
         * @endcond
         */
        void attachPass(std::string passName);

        /** @} */

        /** @name 资源分配与渲染命令
         *  @{
         */

        /**
         * @cond CHINESE
         * @brief 根据已定义的 UniformSlot 分配一个 Uniform 实例。
         * @param name UniformSlot 的名称。
         * @return 返回一个 Uniform 封装对象，可用于后续数据更新和绑定。
         * @endcond
         */
        Uniform allocateUniform(std::string name);

        /**
         * @cond CHINESE
         * @brief 开始一个渲染批次。
         * @note 在调用一系列 bind* 和 draw* 命令之前必须调用此函数。它会重置当前帧的临时绑定状态。
         * @endcond
         */
        void begin();

        /**
         * @cond CHINESE
         * @brief 绑定一个 Uniform 常量缓冲区。
         * @param uniform 通过 allocateUniform 创建的 Uniform 对象。
         * @endcond
         */
        void bindUniform(const Uniform& uniform);
        void bindUniform(Uniform* uniform);

        /**
         * @cond CHINESE
         * @brief 绑定一个纹理。
         * @param name 纹理在着色器中的名称。
         * @param image 指向 FCT::Image 对象的指针。
         * @endcond
         */
        void bindTexture(std::string name, FCT::Image* image);

        /**
         * @cond CHINESE
         * @brief 绑定一个采样器。
         * @param name 采样器在着色器中的名称。
         * @param sampler 指向 FCT::Sampler 对象的指针。
         * @endcond
         */
        void bindSampler(std::string name, FCT::Sampler* sampler);

        /**
         * @cond CHINESE
         * @brief 绑定顶点着色器。
         * @param code 顶点着色器的源码字符串。
         * @endcond
         */
        void bindVertexShader(std::string code);

        /**
         * @cond CHINESE
         * @brief 绑定像素着色器。
         * @param code 像素着色器的源码字符串。
         * @endcond
         */
        void bindPixelShader(std::string code);

        /**
         * @cond CHINESE
         * @brief 通过着色器引用绑定顶点着色器。
         * @param ref 通过 cacheVertexShader 获取的着色器引用。
         * @endcond
         */
        void bindVertexShader(const ShaderRef& ref);
  /**
         * @cond CHINESE
         * @brief 通过着色器引用绑定像素着色器。
         * @param ref 通过 cachePixelShader 获取的着色器引用。
         * @endcond
         */
        void bindPixelShader(const ShaderRef& ref);
        void bindDepthStencilState(FCT::DepthStencilState* state);
        void bindBlendState(FCT::BlendState* state);
        void bindRasterizationState(FCT::RasterizationState* state);

        /**
         * @cond CHINESE
         * @brief 绘制一个网格。
         * @tparam T 网格类型，通常是 FCT::StaticMesh 或 FCT::DynamicMesh。
         * @param cmdBuffer RHI 命令缓冲区。
         * @param mesh 指向要绘制的网格对象的指针。
         * @endcond
         */
        template<typename T>
        void drawMesh(RHI::CommandBuffer* cmdBuffer,T* mesh);

        template<typename T>
        void drawMeshIndirect(RHI::CommandBuffer* cmdBuffer, T* mesh, void* indirectBuffer, size_t byteOffset, uint32_t drawCount = 1);

        /**
         * @cond CHINESE
         * @brief 绘制一个网格。
         * @tparam T 网格类型，通常是 FCT::StaticMesh 或 FCT::DynamicMesh。
         * @param cmdBuffer RHI 命令缓冲区。
         * @param mesh 对要绘制的网格对象的引用。
         * @endcond
         */
        template<typename T>
        void drawMesh(RHI::CommandBuffer* cmdBuffer,T& mesh);

        /**
         * @cond CHINESE
         * @brief 结束渲染批次。
         * @note 必须与 begin() 配对使用。
         * @endcond
         */
        void end();

        /** @} */

        /** @name 缓存管理
         *  @{
         */

        /**
         * @cond CHINESE
         * @brief 预缓存一个顶点着色器并返回其引用。
         * @param code 顶点着色器的 GLSL 源码。
         * @return ShaderRef 着色器引用，可用于后续的 bindVertexShader 调用。
         * @endcond
         */
        ShaderRef cacheVertexShader(const std::string& code);

        /**
         * @cond CHINESE
         * @brief 预缓存一个像素着色器并返回其引用。
         * @param code 像素着色器的 GLSL 源码。
         * @return ShaderRef 着色器引用，可用于后续的 bindPixelShader 调用。
         * @endcond
         */
        ShaderRef cachePixelShader(const std::string& code);

        /**
         * @cond CHINESE
         * @brief 清空所有已缓存的 PassResource。
         * @endcond
         */
        void clearPassResourceCache();
// ... existing code
        /**
         * @cond CHINESE
         * @brief 清空所有已缓存的渲染管线。
         * @endcond
         */
        void clearPipelineCache();

        /**
         * @cond CHINESE
         * @brief 清空所有已缓存的着色器。
         * @endcond
         */
        void clearShaderCache();

        /** @} */

    private:
        // =================================================================================
        // 私有辅助结构体 (Private Helper Structs)
        // =================================================================================

        // 管线状态
        struct TraditionPipelineState
        {
            FCT::VertexShader* vertexShader = nullptr;
            FCT::PixelShader* pixelShader = nullptr;
            FCT::BlendState* blendState = nullptr;
            FCT::RasterizationState* rasterizationState = nullptr;
            FCT::DepthStencilState* depthStencilState = nullptr;
            size_t hash() const;
            bool operator==(const TraditionPipelineState& other) const;
        };

        // Pass 资源状态
        struct PassResourceState
        {
            std::unordered_map<std::string, FCT::Image*> boundTextures;
            std::unordered_map<std::string, FCT::Sampler*> boundSamplers;
            std::unordered_map<std::string, FCT::RHI::ConstBuffer*> boundUniforms;
            void clear();
            void bindTexture(const std::string& name, FCT::Image* image);
            void bindSampler(const std::string& name, FCT::Sampler* sampler);
            void bindUniform(const std::string& name, const Uniform& uniform);
            void bindUniform(const std::string& name, Uniform* uniform);
            size_t hash() const;
        };

        // Pass 资源缓存
        struct PassResourceCache
        {
            std::unordered_map<size_t,FCT::PassResource*> m_passResources;
            ~PassResourceCache();
            FCT::PassResource* get(const PassResourceState& state, const std::function<FCT::PassResource*(const PassResourceState& state)>& creator);
        };

        // 管线缓存
        struct PipelineCache
        {
            std::unordered_map<size_t, FCT::RHI::RasterizationPipeline*> m_pipelines;
            ~PipelineCache();
            FCT::RHI::RasterizationPipeline* get(const TraditionPipelineState& state, const std::function<FCT::RHI::RasterizationPipeline*(const TraditionPipelineState& state)>& creator);
        };

        // 着色器缓存
        struct ShaderCache
        {
            std::unordered_map<size_t, FCT::VertexShader*> m_vertexShaders;
            std::unordered_map<size_t, FCT::PixelShader*> m_pixelShaders;
            ~ShaderCache();
            FCT::VertexShader* getVertexShader(const std::string& code,const std::function<FCT::VertexShader*(const std::string& code)>& creator);
            FCT::PixelShader* getPixelShader(const std::string& code,const std::function<FCT::PixelShader*(const std::string& code)>& creator);
            FCT::VertexShader* getVertexShader(const ShaderRef& ref,const std::function<FCT::VertexShader*(const ShaderRef& ref)>& creator);
            FCT::PixelShader* getPixelShader(const ShaderRef& ref, const std::function<FCT::PixelShader*(const ShaderRef& ref)>& creator);
            void clear();
        };

        // =================================================================================
        // 私有成员变量 (Private Member Variables)
        // =================================================================================

        // --- 核心上下文与布局定义 ---
        FCT::Context* m_ctx;
        std::map<uint32_t, FCT::VertexLayout> m_vertexLayouts;
        std::map<std::string, FCT::ConstLayout> m_uniformLayouts;
        FCT::PixelLayout m_pixelLayout;
        FCT::ResourceLayout m_resourceLayout;
        FCT::ResourceLayout m_vertexResourceLayout;
        FCT::ResourceLayout m_pixelResourceLayout;
        std::unordered_map<std::string, FCT::Image*> m_fixedImages;
        std::map<std::string, Image*> m_textureFromPass;
        std::unordered_map<std::string,const char*> m_textureNames;

        // --- 状态与资源管理 ---
        uint32_t m_nextAvailableIndex = 0;
        std::vector<TextureSlot> m_unhandledTextureSlots;
        bool m_hasVertexLayout = false;
        bool m_hasPixelLayout = false;

        // --- 当前 Pass/Pipeline 状态 ---
        bool m_injectPassResource = true;
        std::string m_passName;
        FCT::RHI::Pass* m_pass = nullptr;
        std::pair<SubscribeId, SubscribeId> m_passPipeSubIds{0, 0};
        PassResourceState m_passResourceState;
        TraditionPipelineState m_pipelineState;

        // --- 缓存对象 ---
        PassResourceCache m_passResourceCache;
        PipelineCache m_pipelineCache;
        ShaderCache m_shaderCache;

        // =================================================================================
        // 私有辅助函数 (Private Helper Functions)
        // =================================================================================

        // --- 构造函数参数处理 ---
        template<typename... Args>
        void proccessArgs(FCT::VertexLayout vertexLayout, Args... args);
        template <class ... Args>
        void proccessArgs(const std::vector<FCT::VertexLayout>& vertexLayouts, Args... args);
        template<typename... Args>
        void proccessArgs(SamplerSlot samplerSlot, Args... args);
        template<typename... Args>
        void proccessArgs(PassName passName, Args... args);
        template<typename... Args>
        void proccessArgs(uint32_t index, FCT::VertexLayout vertexLayout, Args... args);
        template<typename... Args>
        void proccessArgs(FCT::PixelLayout pixelLayout, Args... args);
        template<typename... Args>
        void proccessArgs(TextureSlot textureSlot, Args... args);
        template<typename... Args>
        void proccessArgs(UniformSlot uniformSlot, Args... args);
        template<typename... Args>
        void proccessArgs(DisableInjectPassResource disable, Args... args);
        void proccessArgs();

        // --- 内部资源获取 ---
        FCT::PassResource* getCurrentPassResource();
        FCT::RHI::RasterizationPipeline* getCurrentPipeline();

        // --- 内部绑定与分配 ---
        void bindVertexShader(FCT::VertexShader* shader);
        void bindPixelShader(FCT::PixelShader* shader);
        FCT::VertexShader* allocateVertexShader(std::string code);
        FCT::PixelShader* allocatePixelShader(std::string code);

        // --- 缓存访问 ---
        VertexShader* getCacheVertexShader(std::string code);
        PixelShader* getCachePixelShader(std::string code);
        VertexShader* getCacheVertexShader(const ShaderRef& ref);
        PixelShader* getCachePixelShader(const ShaderRef& ref);

        // --- 其他工具函数 ---
        void processUnhandledTextureSlots();
        uint32_t findNextAvailableIndex();
    };

}
#endif //LAYOUT_H