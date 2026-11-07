#ifndef FCT_CONTEXT_IH
#define FCT_CONTEXT_IH
#include "../UI/Window.h"
namespace FCT {


	inline void Context::submitThread()
	{
		while (m_ctxRunning.load(std::memory_order_relaxed)) {
			_waitForNextFrame();
			if (!m_ctxRunning.load(std::memory_order_acquire)) {
				break;
			}
			{
				ZoneScopedN("SubmitFrame");
				m_flowControl->executeSubmitTickers();
				_currentFlush();
			}
			FrameMarkNamed("RenderSubmit");
		}
	}
	inline void Context::flush()
	{
		ZoneScoped();
		_consumeLogicTask();
	    _waitCurrentFlush();
		if (!m_ctxRunning.load(std::memory_order_acquire))
		{
			return;
		}
		m_flowControl->executeSyncTickers();
		_nextFrame();
	}

	inline void Context::maxFrameInFlight(uint32_t maxFrameInFlight)
	{
		m_maxFrameInFlight = (maxFrameInFlight == 0) ? 1u : maxFrameInFlight;
		if (m_cmdGraph)
		{
			m_cmdGraph->maxFrameInFlight(m_maxFrameInFlight);
		}
	}

	inline RHI::DescriptorPool* Context::getDescriptorPool()
	{
		return m_descriptorPool;
	}

	template <typename T>
	inline T* Context::createResource()
	{
		return m_resourceDevice->createResource<T>();
	}
}
#endif