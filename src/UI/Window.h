#ifndef FCT_WINDOW_H
#define FCT_WINDOW_H
#include "../MutilThreadBase/RefCount.h"
#include "../Context/DataTypes.h"
#include "../Context/IRenderTarget.h"
#include "./EventHandler.h"
#include "./CallBackHandler.h"
#include "../Context/ImageRenderTarget.h"
#include "../RHI/Semaphore.h"
#include "../RHI/Swapcain.h"
#include "../DebugTools/OutStream.h"
#include "./AutoViewport.h"
#include "./InputStateEventHanndler.h"
#include "../Context/Context.h"
#include "../Base/IModuleManager.h"

namespace FCT {
	namespace RHI
	{
		class Swapchain;
	}

	class WindowBehavior
	{
    public:
        virtual void pos(int x,int y) = 0;
        virtual void size(int w, int h) = 0;
		virtual void title(const std::string& title) = 0;
    private:

    };
    class SetParamWindowBehavior;
	class EventHandler;
    using VFuncII = std::function<void(int,int)>;
	class SwapchainTargetWrapper
	{
	public:
		virtual ~SwapchainTargetWrapper() = default;
		virtual void* getNativeHandler() = 0;
	private:
	};
	namespace WindowModule
	{
		struct Swapchain {
			Context* ctx;
			explicit Swapchain(Context* context) : ctx(context) {}
		};

		struct AutoViewport {
			uint32_t width, height;
			AutoViewport(uint32_t w, uint32_t h) : width(w), height(h) {}
		};

		struct DepthStencil {
			Format format;
			explicit DepthStencil(Format fmt) : format(fmt) {}
		};
	}
	enum class WindowCreateFlag
	{
		withDefaultDepthStencil = 1 << 0,
		withAutoViewport = 1 << 1,
		defaultConfig = withDefaultDepthStencil | withAutoViewport
	};
	FCT_DECLARE_FLAGS(WindowCreateFlag);
	class Window : public RefCount,public IRenderTarget {
    protected:
        WindowBehavior* m_behavior;
		virtual SwapchainTargetWrapper* getSwapchainTarget(Context* ctx) = 0;
    public:
        friend class SetParamWindowBehavior;
		RenderTargetType getType() const override { return RenderTargetType::WindowTarget; }
		Window();
		virtual ~Window() {
			clearHandler();
			m_callbackHandler->release();
		}
        void pos(int x, int y) {
            m_behavior->pos(x, y);
        }
        void size(int w, int h) {
            m_behavior->size(w, h);
        }
        virtual void createPlatform() = 0;
        virtual void destroyPlatform() {}
        void destroy();
		void create(WindowCreateFlags flags = WindowCreateFlag::defaultConfig);
        void bind(Context* ctx) override;
        void setContext(Context* ctx) { m_ctx = ctx; }
        Context* getContext() const { return m_ctx; }
        virtual bool isRunning() const = 0;
        bool isVisible() const { return m_visible; }
        virtual void setVisible(bool visible) { m_visible = visible; }
		virtual void swapBuffers() = 0;
		virtual int getWidth() = 0;
		virtual int getHeight() = 0;
        //virtual void viewport(Vec2 lt, Vec2 rb) = 0;
		virtual void setCursorPos(int x, int y) = 0;
		void registerHandler(EventHandler* handler);
		void unregisterHandler(EventHandler* handler);
		virtual Vec2 getCursorPos() const = 0;
		void clearHandler();
		Image* targetImage() const
		{
			return m_swapchain->target()->targetImage();
		}
		CallBackEventHandler* getCallBack() const {
			return m_callbackHandler;
		}
		ImageRenderTarget* getCurrentTarget() {
			return m_swapchain->target();
		}
		void addRenderFinshSemaphore(RHI::Semaphore* semaphore)
		{
			if (m_swapchain)
			{
				m_swapchain->addRenderFinshSemaphore(semaphore);
				return;
			}
			ferr << "未 bind Context" << std::endl;
		}
		void clearRenderFinshSemaphores()
		{
			m_swapchain->clearRenderFinshSemaphores();
		}
		void setPresentFinshSemaphore(RHI::Semaphore* semaphore)
		{
			m_swapchain->setPresentFinshSemaphore(semaphore);
		}
		void addRenderFinshFence(RHI::Fence* fence)
		{
			m_swapchain->addRenderFinshFence(fence);
		}
		void clearRenderFinshFences()
		{
			m_swapchain->clearRenderFinshFences();
		}
		void initRender();
		void title(const std::string& title)
		{
            m_behavior->title(title);
		}
		//目前只能在bind前 调用,bind后调用为未定义 行为
		void enableDepthBuffer(Format format)
		{
			if (m_swapchain)
			{
				m_swapchain->enableDepthBuffer(format);
				m_needEnableDepthBuffer = false;
			} else
			{
				m_needEnableDepthBuffer = true;
				m_depthBufferFormat = format;
			}
		}
		uint32_t getSwapchainImageCount() const
		{
			return m_swapchain->getImageCount();
		}
		Samples getSwapchainSampleCount() const
		{
			return m_swapchain->getSampleCount();
		}
		RHI::Swapchain* swapchain() const
		{
			return m_swapchain;
		}
		template<typename... Components>
	    void addModule(Components&&... components) {
			(addSingleComponent(std::forward<Components>(components)), ...);
		}

		template<typename Component>
		auto getModule() const -> std::conditional_t<
			std::is_same_v<Component, WindowModule::Swapchain>, RHI::Swapchain*,
			std::conditional_t<
				std::is_same_v<Component, WindowModule::AutoViewport>, AutoViewport*,
				std::conditional_t<
					std::is_same_v<Component, WindowModule::DepthStencil>, Format,
					void*
				>
			>
		> {
			if constexpr (std::is_same_v<Component, WindowModule::Swapchain>) {
				return m_swapchain;
			} else if constexpr (std::is_same_v<Component, WindowModule::AutoViewport>) {
				return m_autoViewport;
			} else if constexpr (std::is_same_v<Component, WindowModule::DepthStencil>) {
				return m_depthBufferFormat;
			} else {
				static_assert(sizeof(Component) == 0, "Unsupported module type");
				return nullptr;
			}
		}
	private:
	protected:
		AutoViewport* m_autoViewport;
		CallBackEventHandler* m_callbackHandler;
		std::vector<EventHandler*> m_handlers;
        int m_x,m_y, m_width, m_height;
		std::string m_title;
        RHI::Swapchain* m_swapchain;
		bool m_needEnableDepthBuffer;
		Format m_depthBufferFormat;
        Context* m_ctx;
		EventDispatcher<EventSystemConfig::IdentifierTriggerOnly> m_delayModuleCreate;
		bool m_visible{true};
	private:
		void addSingleComponent(const WindowModule::Swapchain& component);
        void addSingleComponent(const WindowModule::AutoViewport& component);
        void addSingleComponent(const WindowModule::DepthStencil& component);
	};


	class SetParamWindowBehavior : public WindowBehavior {
    private:
        Window* m_window;
    public:
        SetParamWindowBehavior(Window* window) : m_window(window) {}
        void pos(int x, int y) override {
            m_window->m_x = x;
            m_window->m_y = y;
        }
        void size(int w, int h) override {
            m_window->m_width = w;
            m_window->m_height = h;
        }
    	void title(const std::string& title) override {
            m_window->m_title = title;
        }
    };
    inline Window::Window() {
    	m_autoViewport = nullptr;
    	m_swapchain = nullptr;
    	m_needEnableDepthBuffer = false;
        m_behavior = new SetParamWindowBehavior(this);
        m_callbackHandler = new CallBackEventHandler();
    	m_callbackHandler->addResizeCallback([this](Window*,int w,int h)
    	{
    		if (m_swapchain)
    		{
    			//m_swapchain->size(w,h);
    			m_swapchain->needRecreate(w,h);
    		}
    	});
        registerHandler(m_callbackHandler);
    }

}
#endif
