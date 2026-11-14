#pragma once
#ifndef FCT_RHI_PIPELINE_H
#define FCT_RHI_PIPELINE_H

#include "../ThirdParty.h"
#include "../MutilThreadBase/RefCount.h"
#include "../Base/Status.h"
#include <cstdint>

namespace FCT
{
    /**
     * @brief 硬件渲染管线类型枚举
     */
    enum class PipelineType : uint8_t
    {
        Traditional = 0, // 传统固定输入布局光栅化管线 (IA + VS + PS)
        MeshShader = 1,  // 基于网格着色器的几何放大管线 (Task + Mesh + PS)
        RayTracing = 2,  // 硬件光线追踪管线 (RayGen + Miss + HitGroup + Callable)
        Compute = 3      // 通用计算管线 (CS)
    };

    namespace RHI
    {
        class CommandBuffer;

        /**
         * @brief 底层 RHI 管线基类纯虚契约
         * 为所有硬件图形管线提供统一的多态绑定与驱动句柄访问规范
         */
        class Pipeline
        {
        public:
            Pipeline() = default;
            virtual ~Pipeline() = default;

            Pipeline(const Pipeline&) = delete;
            Pipeline& operator=(const Pipeline&) = delete;
            Pipeline(Pipeline&&) noexcept = default;
            Pipeline& operator=(Pipeline&&) noexcept = default;

            virtual void release()
            {
                FCT_DELETE(this);
            }

            /**
             * @brief 获取管线具体类型
             */
            virtual PipelineType getType() const noexcept = 0;

            /**
             * @brief 绑定管线至指定命令缓冲区
             */
            virtual void bind(CommandBuffer* cmdBuf) = 0;

            /**
             * @brief 创建并实例化硬件管线状态对象 (PSO)
             * @return 运行期状态，零异常控制
             */
            virtual Status create() = 0;

            /**
             * @brief 获取底层原生管线句柄（转换为 64 位整数）
             */
            virtual uint64_t getNativeHandle() const noexcept { return 0; }

            /**
             * @brief 获取底层原生管线布局句柄（转换为 64 位整数）
             */
            virtual uint64_t getNativeLayoutHandle() const noexcept { return 0; }
        };
    }
}

#endif // FCT_RHI_PIPELINE_H
