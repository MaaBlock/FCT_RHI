#include "MiniGame_Window.h"
#include "../Runtime/runtime.h"

namespace FCT {

MiniGame_Window::MiniGame_Window(Runtime* rt, int cssWidth, int cssHeight, float systemDpr, float dprCap)
    : m_runtime(rt), m_cssWidth(cssWidth), m_cssHeight(cssHeight), m_systemDpr(systemDpr), m_dprCap(dprCap) {
    recalculateDimensions();
}

MiniGame_Window::~MiniGame_Window() {
    destroyPlatform();
}

void MiniGame_Window::createPlatform() {
    recalculateDimensions();
    m_running = true;
}

void MiniGame_Window::destroyPlatform() {
    m_running = false;
}

void MiniGame_Window::swapBuffers() {
    if (m_swapchain) {
        m_swapchain->present();
    }
}

void MiniGame_Window::setDprCap(float cap) {
    if (cap > 0.0f && std::abs(m_dprCap - cap) > 1e-4f) {
        m_dprCap = cap;
        recalculateDimensions();
        if (m_callbackHandler) {
            m_callbackHandler->invokeResizeCallbacks(this, m_width, m_height);
        }
        if (m_swapchain) {
            m_swapchain->needRecreate(m_width, m_height);
        }
    }
}

void MiniGame_Window::onCanvasResize(int cssWidth, int cssHeight, float systemDpr) {
    m_cssWidth = cssWidth;
    m_cssHeight = cssHeight;
    m_systemDpr = systemDpr;
    recalculateDimensions();
    if (m_callbackHandler) {
        m_callbackHandler->invokeResizeCallbacks(this, m_width, m_height);
    }
    if (m_swapchain) {
        m_swapchain->needRecreate(m_width, m_height);
    }
}

void MiniGame_Window::simulateCanvasEnvironment(int cssWidth, int cssHeight, float systemDpr, float dprCap) {
    m_dprCap = dprCap;
    onCanvasResize(cssWidth, cssHeight, systemDpr);
}

void MiniGame_Window::setCursorPos(int x, int y) {
    m_cursorPos = Vec2(static_cast<float>(x), static_cast<float>(y));
}

Vec2 MiniGame_Window::getCursorPos() const {
    return m_cursorPos;
}

Vec2 MiniGame_Window::cssToPhysical(float cssX, float cssY) const noexcept {
    return Vec2(cssX * m_effectiveDpr, cssY * m_effectiveDpr);
}

Vec2 MiniGame_Window::physicalToCss(float physX, float physY) const noexcept {
    if (m_effectiveDpr <= 0.0f) {
        return Vec2(physX, physY);
    }
    return Vec2(physX / m_effectiveDpr, physY / m_effectiveDpr);
}

SwapchainTargetWrapper* MiniGame_Window::getSwapchainTarget(Context* /*ctx*/) {
    return new MiniGame_SwapchainTargetWrapper();
}

void MiniGame_Window::recalculateDimensions() {
    m_effectiveDpr = (m_dprCap > 0.0f) ? std::min(m_systemDpr, m_dprCap) : m_systemDpr;
    m_width = static_cast<int>(std::floor(static_cast<float>(m_cssWidth) * m_effectiveDpr));
    m_height = static_cast<int>(std::floor(static_cast<float>(m_cssHeight) * m_effectiveDpr));
    if (m_width < 1) m_width = 1;
    if (m_height < 1) m_height = 1;
}

} // namespace FCT
