#include "Emscripten_Runtime.h"
#include "../UI/CallBackHandler.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>

static void emscriptenMainLoopCallback(void* userData) {
    auto* runtime = static_cast<FCT::Emscripten_Runtime*>(userData);
    if (runtime && !runtime->isPaused()) {
        double dt = 1.0 / static_cast<double>(runtime->getTargetFps() > 0 ? runtime->getTargetFps() : 60);
        runtime->stepFrame(dt);
    }
}
#endif

namespace FCT {

Emscripten_Runtime::Emscripten_Runtime() {
}

Emscripten_Runtime::~Emscripten_Runtime() {
    stopMainLoop();
}

void Emscripten_Runtime::startMainLoop(std::function<void(double dt)> tickCallback, int targetFps) {
    m_tickCallback = std::move(tickCallback);
    m_targetFps = targetFps;
    m_loopRunning = true;
    m_paused = false;
#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg(emscriptenMainLoopCallback, this, targetFps, 0);
#endif
}

void Emscripten_Runtime::stopMainLoop() {
    m_loopRunning = false;
    m_paused = false;
#if defined(__EMSCRIPTEN__)
    emscripten_cancel_main_loop();
#endif
}

void Emscripten_Runtime::pauseMainLoop() {
    m_paused = true;
#if defined(__EMSCRIPTEN__)
    emscripten_pause_main_loop();
#endif
}

void Emscripten_Runtime::resumeMainLoop() {
    m_paused = false;
#if defined(__EMSCRIPTEN__)
    emscripten_resume_main_loop();
#endif
}

void Emscripten_Runtime::stepFrame(double dt) {
    if (!m_loopRunning || m_paused) {
        return;
    }
    if (m_tickCallback) {
        m_tickCallback(dt);
    }
}

void Emscripten_Runtime::dispatchMouseEvent(TouchPhase phase, float x, float y) {
    if (!m_targetWindow || !m_targetWindow->getCallBack()) {
        return;
    }
    auto* cb = m_targetWindow->getCallBack();
    int ix = static_cast<int>(x);
    int iy = static_cast<int>(y);
    switch (phase) {
    case TouchPhase::Began:
        cb->onLButtonDown(m_targetWindow, ix, iy);
        break;
    case TouchPhase::Moved:
        cb->onMouseMove(m_targetWindow, ix, iy);
        break;
    case TouchPhase::Ended:
    case TouchPhase::Canceled:
        cb->onLButtonUp(m_targetWindow, ix, iy);
        break;
    }
}

void Emscripten_Runtime::injectTouchStart(int32_t id, float x, float y, float force) {
    TouchPoint pt{id, x, y, x, y, force};
    m_activeTouches[id] = pt;

    if (m_primaryTouchId == -1) {
        m_primaryTouchId = id;
    }

    if (m_primaryTouchId == id) {
        dispatchMouseEvent(TouchPhase::Began, x, y);
    }

    if (m_targetWindow && m_targetWindow->getCallBack()) {
        std::vector<TouchPoint> allTouches;
        allTouches.reserve(m_activeTouches.size());
        for (const auto& pair : m_activeTouches) {
            allTouches.push_back(pair.second);
        }
        TouchEvent te{TouchPhase::Began, std::move(allTouches), {pt}};
        m_targetWindow->getCallBack()->invokeTouchCallbacks(m_targetWindow, te);
    }
}

void Emscripten_Runtime::injectTouchMove(int32_t id, float x, float y, float force) {
    auto it = m_activeTouches.find(id);
    if (it == m_activeTouches.end()) {
        return;
    }
    it->second.physicalX = x;
    it->second.physicalY = y;
    it->second.logicalX = x;
    it->second.logicalY = y;
    it->second.force = force;
    TouchPoint pt = it->second;

    if (m_primaryTouchId == id) {
        dispatchMouseEvent(TouchPhase::Moved, x, y);
    }

    if (m_targetWindow && m_targetWindow->getCallBack()) {
        std::vector<TouchPoint> allTouches;
        allTouches.reserve(m_activeTouches.size());
        for (const auto& pair : m_activeTouches) {
            allTouches.push_back(pair.second);
        }
        TouchEvent te{TouchPhase::Moved, std::move(allTouches), {pt}};
        m_targetWindow->getCallBack()->invokeTouchCallbacks(m_targetWindow, te);
    }
}

void Emscripten_Runtime::injectTouchEnd(int32_t id, float x, float y) {
    TouchPoint pt{id, x, y, x, y, 0.0f};
    m_activeTouches.erase(id);

    if (m_primaryTouchId == id) {
        dispatchMouseEvent(TouchPhase::Ended, x, y);
        if (!m_activeTouches.empty()) {
            m_primaryTouchId = m_activeTouches.begin()->first;
        } else {
            m_primaryTouchId = -1;
        }
    }

    if (m_targetWindow && m_targetWindow->getCallBack()) {
        std::vector<TouchPoint> allTouches;
        allTouches.reserve(m_activeTouches.size());
        for (const auto& pair : m_activeTouches) {
            allTouches.push_back(pair.second);
        }
        TouchEvent te{TouchPhase::Ended, std::move(allTouches), {pt}};
        m_targetWindow->getCallBack()->invokeTouchCallbacks(m_targetWindow, te);
    }
}

void Emscripten_Runtime::injectTouchCancel(int32_t id, float x, float y) {
    TouchPoint pt{id, x, y, x, y, 0.0f};
    m_activeTouches.erase(id);

    if (m_primaryTouchId == id) {
        dispatchMouseEvent(TouchPhase::Canceled, x, y);
        if (!m_activeTouches.empty()) {
            m_primaryTouchId = m_activeTouches.begin()->first;
        } else {
            m_primaryTouchId = -1;
        }
    }

    if (m_targetWindow && m_targetWindow->getCallBack()) {
        std::vector<TouchPoint> allTouches;
        allTouches.reserve(m_activeTouches.size());
        for (const auto& pair : m_activeTouches) {
            allTouches.push_back(pair.second);
        }
        TouchEvent te{TouchPhase::Canceled, std::move(allTouches), {pt}};
        m_targetWindow->getCallBack()->invokeTouchCallbacks(m_targetWindow, te);
    }
}

} // namespace FCT
