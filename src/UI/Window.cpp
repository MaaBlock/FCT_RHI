#include "../FCTAPI.h"

void FCT::Window::registerHandler(EventHandler* handler)
{
	m_handlers.push_back(handler);
	handler->addRef();
}

void FCT::Window::unregisterHandler(EventHandler* handler)
{
	auto it = std::find(m_handlers.begin(), m_handlers.end(), handler);
	if (it != m_handlers.end()) {
		m_handlers.erase(it);
	}
	handler->release();
}

void FCT::Window::clearHandler()
{
	for (auto handler : m_handlers) {
		handler->release();
	}
	m_handlers.clear();
}

void FCT::Window::initRender()
{
	m_swapchain->acquireFirstImage();
}

void FCT::Window::addSingleComponent(const WindowModule::Swapchain& component)
{
	m_ctx = component.ctx;
	auto surface = getSwapchainTarget(component.ctx);
	m_swapchain = component.ctx->createResource<RHI::Swapchain>();
	m_swapchain->nativeHandle(surface->getNativeHandler());
	m_swapchain->size(m_width, m_height);
	m_swapchain->create();
	delete surface;
	m_ctx->onWindowBound(this);
	m_delayModuleCreate.trigger<WindowModule::Swapchain>();
}

void FCT::Window::addSingleComponent(const WindowModule::AutoViewport& component)
{
	if (m_swapchain)
	{
		m_autoViewport = new AutoViewport(Vec2(component.width, component.height), Vec2(component.width, component.height));
		m_autoViewport->window(this);
		m_autoViewport->ctx(m_ctx);
		m_autoViewport->enable(true);
	} else
	{
		m_delayModuleCreate.subscribe<WindowModule::Swapchain>([this,component]()
		{
			addModule(component);
		});
	}
}


void FCT::Window::addSingleComponent(const WindowModule::DepthStencil& component)
{
	m_depthBufferFormat = component.format;
	m_needEnableDepthBuffer = true;

	if (m_swapchain)
	{
		enableDepthBuffer(component.format);
	} else
	{
		m_delayModuleCreate.subscribe<WindowModule::Swapchain>([this,component]()
		{
			addModule(WindowModule::DepthStencil(component));
		});
	}
}
namespace FCT
{

	void Window::create(WindowCreateFlags flags)
	{
		createPlatform();
		if (flags & WindowCreateFlag::withDefaultDepthStencil)
		{
			addModule(WindowModule::DepthStencil(Format::D32_SFLOAT_S8_UINT));
		}
		if (flags & WindowCreateFlag::withAutoViewport)
		{
			addModule(WindowModule::AutoViewport(getWidth(), getHeight()));
		}
	}

	void Window::bind(Context* ctx)
	{
		addModule(WindowModule::Swapchain(ctx));
	}

	void Window::destroy()
	{
		if (m_ctx)
		{
			m_ctx->unbindWindow(this);
			m_ctx = nullptr;
		}
		destroyPlatform();
	}
}


