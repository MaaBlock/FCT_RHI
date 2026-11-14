#pragma once
#include "../MutilThreadBase/RefCount.h"
#include <vector>
#include <string>
#include <cstdint>

namespace FCT {
	class Window;

	enum class TouchPhase : uint8_t {
		Began = 0,
		Moved = 1,
		Ended = 2,
		Canceled = 3
	};

	struct TouchPoint {
		int32_t id{0};
		float physicalX{0.0f};
		float physicalY{0.0f};
		float logicalX{0.0f};
		float logicalY{0.0f};
		float force{1.0f};
	};

	struct TouchEvent {
		TouchPhase phase{TouchPhase::Began};
		std::vector<TouchPoint> touches;
		std::vector<TouchPoint> changedTouches;
	};

	class EventHandler : public RefCount {
	public:
		virtual void onResize(Window* wnd,int width,int height) = 0;
		virtual void onMouseMove(Window* wnd, int xpos, int ypos) = 0;
		virtual void onMouseWheel(Window* wnd,int delta) = 0;
		virtual void onLButtonDown(Window* wnd, int xpos, int ypos) = 0;
		virtual void onLButtonUp(Window* wnd, int xpos, int ypos) = 0;
		virtual void onRButtonDown(Window* wnd, int xpos, int ypos) = 0;
		virtual void onRButtonUp(Window* wnd, int xpos, int ypos) = 0;
		virtual void onKeyDown(Window* wnd, int key) = 0;
		virtual void onKeyUp(Window* wnd, int key) = 0;
		virtual void onFileDrop(Window* wnd, const std::vector<std::string>& files) = 0;
		virtual void onTouch(Window* /*wnd*/, const TouchEvent& /*event*/) {}
	private:

	};
}