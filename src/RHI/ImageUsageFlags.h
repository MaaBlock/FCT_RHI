//
// Created by Administrator on 2025/7/28.
//

#ifndef IMAGEUSAGEFLAGS_H
#define IMAGEUSAGEFLAGS_H
#include "../Base/Flags.h"
namespace FCT
{
    enum class ImageUsage : unsigned int
    {
        RenderTarget = 0x1,
        DepthStencil = 0x2,
        Texture      = 0x4,
        Storage      = 0x8,
    };

    FCT_DECLARE_FLAGS(ImageUsage);

    struct ImageInitData
    {
        void* data;
        size_t size;
    };
}
#endif //IMAGEUSAGEFLAGS_H
