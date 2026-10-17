#include "../FCTAPI.h"

namespace FCT {
	struct Win32_RuntimeCommon : public RuntimeCommon {
	
	};
	void Runtime::init() {
		g_common = FCT_NEW(Win32_RuntimeCommon);
	}
	void Runtime::term() {
	    FCT_DELETE(g_common);
	}

	Window* Runtime::createWindow()
	{
		return FCT_NEW(Win32_Window, g_common->win32UICommon, this);
	}

	Window* Runtime::createWindow(WindowBackend backend)
	{
		if (backend == WindowBackend::Win32)
		{
			return FCT_NEW(Win32_Window, g_common->win32UICommon, this);
		}
#ifdef FCT_USE_GLFW
		return FCT_NEW(GLFW_Window, g_common->glfwUICommon, this);
#else
		return FCT_NEW(Win32_Window, g_common->win32UICommon, this);
#endif
	}

	Win32_Window* Runtime::createWin32Window()
	{
		return FCT_NEW(Win32_Window, g_common->win32UICommon, this);
	}

	Win32_Window* Runtime::createWin32Window(int w, int h, const char* title)
	{
		Win32_Window* window = createWin32Window();
		window->size(w, h);
		window->title(title);
		window->create();
		return window;
	}

	Win32_Window* Runtime::createWin32Window(int x, int y, int w, int h, const char* title)
	{
		Win32_Window* window = createWin32Window();
		window->pos(x, y);
		window->size(w, h);
		window->title(title);
		window->create();
		return window;
	}

	Context* Runtime::createContext()
	{
#ifdef FCT_USE_D3D12
		return FCT_NEW(D3D12_Context, g_common->d3d12ContextCommon);
#else
		return FCT_NEW(VK_Context, g_common->vkContextCommon);
#endif
	}

	Context* Runtime::createContext(ContextBackend backend)
	{
#ifdef FCT_USE_D3D12
		if (backend == ContextBackend::D3D12)
		{
			return FCT_NEW(D3D12_Context, g_common->d3d12ContextCommon);
		}
#endif
		return FCT_NEW(VK_Context, g_common->vkContextCommon);
	}

#ifdef FCT_USE_D3D12
	D3D12_Context* Runtime::createD3D12Context()
	{
		return FCT_NEW(D3D12_Context, g_common->d3d12ContextCommon);
	}
#endif

	ImageLoader* Runtime::createImageLoader()
	{
		return FCT_NEW(FreeImage_ImageLoader);
	}

}
