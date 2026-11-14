#pragma once

#include "./runtime.h"
#include "../UI/EventHandler.h"
#include <functional>
#include <map>
#include <vector>

namespace FCT {

/**
 * @brief Emscripten / WebGL2 小游戏异步运行时 (FEAT-41)
 * Note: 提供非阻塞帧循环 (rAF/emscripten_set_main_loop) 与多点触控双轨分发 (Primary->Mouse, Multi->Touch)。
 */
class Emscripten_Runtime : public Runtime {
public:
    explicit Emscripten_Runtime();
    virtual ~Emscripten_Runtime();

    void setTargetWindow(Window* wnd) noexcept { m_targetWindow = wnd; }
    Window* getTargetWindow() const noexcept { return m_targetWindow; }

    void startMainLoop(std::function<void(double dt)> tickCallback, int targetFps = 60);
    void stopMainLoop();
    void pauseMainLoop();
    void resumeMainLoop();
    void stepFrame(double dt);

    bool isLoopRunning() const noexcept { return m_loopRunning; }
    bool isPaused() const noexcept { return m_paused; }
    int getTargetFps() const noexcept { return m_targetFps; }

    // 触控输入注入与双轨分发 (FEAT-41)
    void injectTouchStart(int32_t id, float x, float y, float force = 1.0f);
    void injectTouchMove(int32_t id, float x, float y, float force = 1.0f);
    void injectTouchEnd(int32_t id, float x, float y);
    void injectTouchCancel(int32_t id, float x, float y);

    const std::map<int32_t, TouchPoint>& getActiveTouches() const noexcept { return m_activeTouches; }
    int32_t getPrimaryTouchId() const noexcept { return m_primaryTouchId; }

private:
    void dispatchMouseEvent(TouchPhase phase, float x, float y);

    Window* m_targetWindow{nullptr};
    std::function<void(double dt)> m_tickCallback;
    bool m_loopRunning{false};
    bool m_paused{false};
    int m_targetFps{60};
    double m_lastFrameTime{0.0};
    int32_t m_primaryTouchId{-1};
    std::map<int32_t, TouchPoint> m_activeTouches;
};

} // namespace FCT
