#ifndef FCT_CONTEXT_IH
#define FCT_CONTEXT_IH
#include "../UI/Window.h"
#include "../RHI/StorageBuffer.h"
#include "../RHI/TransformFeedback.h"
namespace FCT {

	inline void Context::requestExit()
	{
		ZoneScopedN("Context::requestExit");
		// 防重入原子保护
		bool expected = false;
		if (!m_exitRequested.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
			return;
		}

		// 1. 先跳出 loop：设置原子退出标志并唤醒/中断等待条件
		stopSubmitThread();
		m_transferRunning.store(false, std::memory_order_release);
		if (m_transferThread.joinable() && m_transferThread.get_id() != std::this_thread::get_id()) {
			m_transferThread.join();
		}

		// 2. 然后等待 GPU 完成所有未决提交与渲染管线
		waitGpu();

		// 3. 然后清理 GPU 资源与延迟释放队列
		cleanupGpuResources();
	}

	inline void Context::waitGpu()
	{
		ZoneScopedN("Context::waitGpu");
		// 基础 Context 默认无需后端特定等待，由派生类（如 VK_Context）重载实现
	}

	inline void Context::cleanupGpuResources()
	{
		ZoneScopedN("Context::cleanupGpuResources");
		if (m_gpuCleanedUp.exchange(true, std::memory_order_acq_rel)) {
			return;
		}
		FCT_SAFE_DELETE(m_renderGraph);
		flushAllDeferredReleases();
	}

	inline void Context::submitThread()
	{
	    _waitForNextFrame();
		while (m_ctxRunning.load(std::memory_order_relaxed)) {
		    ZoneScopedN("SubmitLoop");
			{
				ZoneScopedN("SubmitFrame");
				m_flowControl->executeSubmitTickers();
				_currentFlush();
			}
		    {
		        ZoneScopedN("WaitNextFrame");
		        _waitForNextFrame();
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

	inline RHI::TransformFeedback* Context::createTransformFeedback()
	{
		return createResource<RHI::TransformFeedback>();
	}

	inline RHI::StorageBuffer* Context::createStorageBuffer(size_t size)
	{
		auto* buf = createResource<RHI::StorageBuffer>();
		if (buf && size > 0) {
			(void)buf->create(size);
		}
		return buf;
	}
}
#endif