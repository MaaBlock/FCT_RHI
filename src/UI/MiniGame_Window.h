#pragma once

#include "./Window.h"
#include <cmath>
#include <algorithm>

namespace FCT {

class Runtime;

/**
 * @brief 小游戏 Canvas 表面原生包装 (FEAT-40)
 */
class MiniGame_SwapchainTargetWrapper : public SwapchainTargetWrapper {
public:
    static constexpr uintptr_t CanvasMagicId = 0x43414E56; // "CANV"

    explicit MiniGame_SwapchainTargetWrapper(void* handle = reinterpret_cast<void*>(CanvasMagicId))
        : m_handle(handle) {}

    void* getNativeHandler() override {
        return m_handle;
    }

private:
    void* m_handle;
};

/**
 * @brief 小游戏 Canvas 窗口实现 (FEAT-40)
 * Note: 支持 devicePixelRatioCap 物理降采样，处理 Canvas 缩放与双重坐标系转换。
 */
class MiniGame_Window : public Window {
public:
    MiniGame_Window(Runtime* rt, int cssWidth = 375, int cssHeight = 667, float systemDpr = 2.0f, float dprCap = 2.0f);
    ~MiniGame_Window() override;

    void createPlatform() override;
    void destroyPlatform() override;

    bool isRunning() const override { return m_running; }
    void setRunning(bool running) { m_running = running; }

    void swapBuffers() override;
    int getWidth() override { return m_width; }
    int getHeight() override { return m_height; }
    Image* getImage() const override {
        return (m_swapchain && m_swapchain->target()) ? m_swapchain->target()->targetImage() : nullptr;
    }

    int getCssWidth() const noexcept { return m_cssWidth; }
    int getCssHeight() const noexcept { return m_cssHeight; }
    float getSystemDpr() const noexcept { return m_systemDpr; }
    float getDprCap() const noexcept { return m_dprCap; }
    float getEffectiveDpr() const noexcept { return m_effectiveDpr; }

    void setDprCap(float cap);
    void onCanvasResize(int cssWidth, int cssHeight, float systemDpr);
    void simulateCanvasEnvironment(int cssWidth, int cssHeight, float systemDpr, float dprCap = 2.0f);

    void setCursorPos(int x, int y) override;
    Vec2 getCursorPos() const override;

    Vec2 cssToPhysical(float cssX, float cssY) const noexcept;
    Vec2 physicalToCss(float physX, float physY) const noexcept;

protected:
    SwapchainTargetWrapper* getSwapchainTarget(Context* ctx) override;

private:
    void recalculateDimensions();

    Runtime* m_runtime{nullptr};
    int m_cssWidth{375};
    int m_cssHeight{667};
    float m_systemDpr{2.0f};
    float m_dprCap{2.0f};
    float m_effectiveDpr{2.0f};
    bool m_running{true};
    Vec2 m_cursorPos{0.0f, 0.0f};
};

} // namespace FCT
