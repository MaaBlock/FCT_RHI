#ifndef FCT_RUNTIME_H
#define FCT_RUNTIME_H
#include "../ThirdParty.h"
#include "../UI/Window.h"
#include "../UI/Android_WindowShareData.h"
#include "../ImageLoader/ImageLoader.h"
#include "../Context/VK_ContextCommon.h"
#ifdef FCT_USE_D3D12
#include "../Context/D3D12_ContextCommon.h"
#endif
#include "../ModelLoader/ModelLoader.h"
#ifdef FCT_USE_GLFW
#include "../UI/GLFW_UICommon.h"
#endif
#ifdef FCT_WIN32
#include "../UI/Win32_UICommon.h"
#endif
#ifdef FCT_ANDROID
#include "../UI/Android_UICommon.h"
#endif
#include "../VertexAndText/Freetype_FontCommon.h"
#ifdef FCT_USE_FREEIMAGE
        #include "../ImageLoader/FreeImage_ImageLoader.h"
#else
        #include "../ImageLoader/Stb_ImageLoader.h"
#endif
namespace FCT
{
    class Win32_Window;

    enum class ContextBackend
    {
        Vulkan,
        D3D12,
    };

    enum class WindowBackend
    {
        GLFW,
        Win32,
    };

    struct RuntimeCommon {
#ifdef FCT_USE_VULKAN
        VK_ContextCommon* vkContextCommon;
#endif
#ifdef FCT_USE_D3D12
        D3D12_ContextCommon* d3d12ContextCommon;
#endif
#ifdef FCT_USE_GLFW
    	GLFW_UICommon* glfwUICommon;
#endif
#ifdef FCT_WIN32
        Win32_UICommon* win32UICommon;
#endif
    	Freetype_FontCommon* freetypeFontCommon;
    };
	/*
	 *todo:
	 * 此处 runtime 是给用户包装用 的，允许有多个，
	 * 应当在内部统一隐藏一个单例 内部Runtime 负责真正的Runtiem
	 * 对于一些只能调用一次的Init的Init，term
	 */
    class Runtime {
    public:
        Runtime(){
            init();
#ifdef  FCT_USE_VULKAN
            g_common->vkContextCommon = FCT_NEW(VK_ContextCommon,this);
            g_common->vkContextCommon->init();
#endif
#ifdef  FCT_USE_D3D12
            g_common->d3d12ContextCommon = FCT_NEW(D3D12_ContextCommon,this);
            g_common->d3d12ContextCommon->init();
#endif
#ifdef  FCT_USE_GLFW
        	g_common->glfwUICommon = FCT_NEW(GLFW_UICommon);
#endif
#ifdef  FCT_WIN32
            g_common->win32UICommon = FCT_NEW(Win32_UICommon);
#endif
#ifdef FCT_USE_FREEIMAGE
        	//todo:从FCTv2 直接迁移过来 的，应当改为common的模式
			FreeImage_ImageLoader::Init();
#endif
        	g_common->freetypeFontCommon = FCT_NEW(Freetype_FontCommon);
        	g_common->freetypeFontCommon->init();
        }
        ~Runtime() {
#ifdef  FCT_USE_D3D12
            g_common->d3d12ContextCommon->term();
            FCT_DELETE(g_common->d3d12ContextCommon);
#endif
#ifdef  FCT_USE_VULKAN
            g_common->vkContextCommon->term();
#endif
        	g_common->freetypeFontCommon->tern();
        	FCT_DELETE(g_common->freetypeFontCommon);
#ifdef  FCT_WIN32
            FCT_DELETE(g_common->win32UICommon);
#endif
#ifdef  FCT_USE_GLFW
        	FCT_DELETE(g_common->glfwUICommon);
#endif
        	term();
        }
        void init();
        void term();
        int getDeviceWidth();
        int getDeviceHeight();
        Window* createWindow();
        Window* createWindow(WindowBackend backend);
        Window* createWindow(int w,int h,const char* title = "") {
            Window* window = createWindow();
            window->size(w,h);
        	window->title(title);
            window->create();
            return window;
        }
        Window* createWindow(int w,int h,const char* title,WindowBackend backend) {
            Window* window = createWindow(backend);
            window->size(w,h);
            window->title(title);
            window->create();
            return window;
        }
        Window* createWindow(int x,int y,int w,int h,const char* title = "") {
            Window* window = createWindow();
            window->pos(x,y);
            window->size(w,h);
        	window->title(title);
            window->create();
            return window;
        }
#ifdef FCT_WIN32
        Win32_Window* createWin32Window();
        Win32_Window* createWin32Window(int w, int h, const char* title = "");
        Win32_Window* createWin32Window(int x, int y, int w, int h, const char* title = "");
#endif
        Context* createContext();
        Context* createContext(ContextBackend backend);
#ifdef FCT_USE_D3D12
        D3D12_Context* createD3D12Context();
#endif
    	ImageLoader* createImageLoader();
    	ModelLoader* createModelLoader();
    	Freetype_Font* createFont()
    	{
    		return g_common->freetypeFontCommon->create();
    	}
    	void postUiTask(UITaskFunction task,void* param = nullptr,bool waited = true)
    	{
#ifdef FCT_WIN32
            g_common->win32UICommon->postUiTask(task, param, waited);
#elif defined(FCT_USE_GLFW)
    		g_common->glfwUICommon->postUiTask(task,param, waited);
#endif
    	}
    	PipeHub& pipeHub();
    private:
    	PipeHub m_pipeHub;
        RuntimeCommon* g_common;
    	Runtime(const Runtime&) = delete;
    	Runtime& operator=(const Runtime&) = delete;
    };
}
#endif