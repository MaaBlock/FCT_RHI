//
// Created by Administrator on 2025/7/28.
//

#ifndef CONTEXTEVENT_H
#define CONTEXTEVENT_H
namespace FCT
{
    class Window;
    class Context;
    namespace ContextEvent
    {
        struct WindowBound
        {
            Window* window;
            Context* context;
        };
        struct WindowUnbound
        {
            Window* window;
            Context* context;
        };
    }
}
#endif //CONTEXTEVENT_H
