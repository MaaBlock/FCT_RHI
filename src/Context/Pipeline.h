#pragma once
#ifndef FCT_CONTEXT_PIPELINE_H
#define FCT_CONTEXT_PIPELINE_H

#include "../ThirdParty.h"
#include "../MutilThreadBase/RefCount.h"
#include "../MutilThreadBase/UniquePtr.h"
#include "../RHI/Pipeline.h"
#include "../Base/Status.h"
#include <cassert>

namespace FCT
{
    class Context;

    /**
     * @brief 高层渲染管线抽象基类（Bridge 模式 Abstraction 角色）
     * 统管高层管线业务状态，独占持有底层 RHI 具体驱动实现对象（m_rhiPipeline）
     */
    class Pipeline : public RefCount
    {
    public:
        Pipeline(Context* ctx, PipelineType type, UniquePtr<RHI::Pipeline> rhiPipeline)
            : m_ctx(ctx), m_type(type), m_rhiPipeline(std::move(rhiPipeline))
        {
            assert(m_ctx != nullptr && "Context 不能为空");
            assert(m_rhiPipeline != nullptr && "底层 RHI Pipeline 实例不能为空");
        }

        ~Pipeline() override = default;

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;
        Pipeline(Pipeline&&) noexcept = default;
        Pipeline& operator=(Pipeline&&) noexcept = default;

        /**
         * @brief 获取管线类型
         */
        PipelineType getType() const noexcept { return m_type; }

        /**
         * @brief 获取底层 RHI 管线裸指针
         */
        RHI::Pipeline* getRhiPipeline() const noexcept
        {
            return m_rhiPipeline.get();
        }

        /**
         * @brief 类型安全向下转换为特定 RHI 管线接口
         */
        template <typename SpecificRhiPipeline>
        SpecificRhiPipeline* getRhiPipelineAs() const noexcept
        {
            return static_cast<SpecificRhiPipeline*>(m_rhiPipeline.get());
        }

        /**
         * @brief 绑定管线到目标命令缓冲区
         */
        virtual void bind(RHI::CommandBuffer* cmdBuf)
        {
            assert(m_rhiPipeline != nullptr && "底层 RHI 管线尚未构建完成");
            m_rhiPipeline->bind(cmdBuf);
        }

        /**
         * @brief 构建并初始化管线
         */
        virtual Status build() = 0;

        /**
         * @brief 兼容旧版 create() 接口
         */
        virtual void create()
        {
            auto status = build();
            assert(status.ok() && "管线创建失败");
        }

    protected:
        Context* m_ctx = nullptr;
        PipelineType m_type;
        UniquePtr<RHI::Pipeline> m_rhiPipeline;
    };
} // namespace FCT

#endif // FCT_CONTEXT_PIPELINE_H
