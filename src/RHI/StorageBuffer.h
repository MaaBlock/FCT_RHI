/**
 * @file StorageBuffer.h
 * @brief FCT RHI 存储缓冲区 (Shader Storage Buffer Object - SSBO) 纯虚接口定义
 */
#pragma once

#include <FCT_Base.h>
#include "../MutilThreadBase/RefCount.h"
#include <cstddef>
#include <cstdint>

namespace FCT
{
    namespace RHI
    {
        /**
         * @brief 图形硬件存储缓冲区 (SSBO) 纯虚契约抽象
         * 继承自 FCT::RefCount，遵循引用计数生命周期管理规范
         */
        class StorageBuffer : public RefCount
        {
        public:
            virtual ~StorageBuffer() = default;

            /**
             * @brief 创建并分配指定大小的 GPU 存储缓冲显存
             * @param size 缓冲区字节数
             * @return 成功返回 OkStatus，失败返回对应错误状态
             */
            virtual Status create(size_t size) = 0;

            /**
             * @brief 获取缓冲区的总字节大小
             */
            virtual size_t size() const = 0;

            /**
             * @brief 映射缓冲区显存到主机虚拟地址空间
             * @return 映射后的主机内存虚拟地址指针，失败返回 nullptr
             */
            virtual void* map() = 0;

            /**
             * @brief 解除显存映射
             */
            virtual void unmap() = 0;

            /**
             * @brief 写入/更新缓冲区数据
             * @param data 源数据指针
             * @param size 写入字节数
             * @param offset 目标缓冲区偏移字节数
             */
            virtual void updateData(const void* data, size_t size, size_t offset = 0) = 0;

            /**
             * @brief 兼容上层上传数据统一驱动调用
             */
            void upload(const void* data, size_t size, size_t offset = 0)
            {
                updateData(data, size, offset);
            }

            /**
             * @brief 获取 64 位 GPU 设备物理虚拟地址 (VK_KHR_buffer_device_address)
             * @return 64 位 GPU 设备虚拟地址，未开启或不支持时返回 0
             */
            virtual uint64_t getDeviceAddress() const = 0;
        };
    }
}
