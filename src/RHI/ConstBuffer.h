#include "../Context/Uniform.h"
#ifndef FCT_RHI_CONSTBUFFER_H
#define FCT_RHI_CONSTBUFFER_H
namespace FCT {
    namespace RHI
    {
        class ConstBuffer
        {
        public:
            virtual ~ConstBuffer() = default;

            void layout(const ConstLayout& layout)
            {
                m_layout = layout;
            }
            void updataData()
            {
                mapData();
            }
            virtual void mapData() {}
            virtual void create() = 0;
            virtual void upload(const void* data, size_t size) = 0;
            virtual void upload(const void* data, size_t size, size_t offset) = 0;

            // Note: m_buffer 已解耦移除，空实现以兼容旧调用
            void buffer(UniformBuffer* /*buffer*/) {}
            UniformBuffer* getBuffer() const { return nullptr; }
            ConstLayout layout() const { return m_layout; }
        protected:
            ConstLayout m_layout;
        };
    }
}
#endif //FCT_RHI_CONSTBUFFER_H
