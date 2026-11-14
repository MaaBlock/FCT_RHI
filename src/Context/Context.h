#pragma once
#include "../ThirdParty.h"
#include "../ToolDefine.h"
#include "../Bases.h"
#include "../Memory/ObjectPool.h"
#include "./RenderGraph.h"
#include "../MutilThreadBase/Computation.h"
#include "./DataTypes.h"
#include "../RHI/VertexShader.h"
#include "../RHI/PixelShader.h"
#include "../RHI/Swapcain.h"
#include "./Material.h"
#include "./DrawCall.h"
#include "./ConstBuffer.h"
#include "../ImageLoader/ImageLoader.h"
#include "../Type/type.h"
#include "./Image.h"
#include "../RHI/CommandPool.h"
#include "../RHI/Pass.h"
#include "../RHI/Fence.h"
#include "../RHI/Semaphore.h"
#include "../RHI/Sampler.h"
#include "../RHI/InputLayout.h"
#include "../RHI/DescriptorPool.h"
#include "./ShaderCompiler.h"
#include "./ShaderGenerator.h"
#include "../ModelLoader/ModelLoader.h"
// Legacy OldRenderGraph removed
#include "FencePool.h"
#include "SemaphorePool.h"
#include "./Device.h"
#include "Mesh.h"
#include "../Base/TokenGraph.h"
#include "../Base/IEventSystem.h"
#include "./ResourceManager.h"
#include "./ContextEvent.h"
#include "./RenderGraph.h"
#include "./CommandBufferGraph.h"
#include "./SubmitThreadManager.h"
#include "./FlowControl.h"
namespace FCT
{
	class RasterizationState;

	class BlendState;
	class TraditionalPipeline;
	class MeshShaderPipeline;
	class RayTracingPipeline;
	enum class PipelineType : uint8_t;
	namespace RHI
	{
		class Pipeline;
		class RasterizationPipeline;
		class TextureView;
		class ConstBuffer;
		class IndexBuffer;
		class StorageBuffer;
		class TransformFeedback;
	}
    class Sampler;

	class VertexBuffer;
	//class InputLayout;
	class VertexShader;
	class PixelShader;
	class DrawCall;
	class Window;
	class SemaphorePool;
	class FencePool;
	class RenderGraph;

	using TickerToken = uint32_t;
	/*Context作用
	 *1.作为Context接口
	 *2.作为RenderGraph系统
	 *3.作为帧 管理器（合并在RenderGraph系统）
	 */
	constexpr const char* RenderGraphSubmitTickerName = "RenderGraphSubmitTicker";
	constexpr const char* RenderGraphExcutePassSubmitTickerName = "RenderGraphExcutePassSubmitTicker";
	constexpr const char* SwapBufferSubmitTicker = "SwapBufferSubmitTicker";
	constexpr const char* RenderGraphSyncTicker_SwapJobQueueName = "RenderGraphSyncTicker_SwapJobQueue";
	namespace InnerSync
	{
		constexpr const char* CheckRecreateSwapchainSync = "CheckRecreateSwapchainSync";
		constexpr const char* AdvanceFrameIndex = "AdvanceFrameIndex";
	}
	namespace ContextTicker
	{

	}
	/**
	 *@note successors of RenderGraphSubmitTicker has RenderGraphExcutePassSubmitTickerName SwapBufferSubmitTicker
	 *		successors of RenderGraphExcutePassSubmitTickerName has SwapBufferSubmitTicker
	 *		if you want to
	 */
	class Runtime;
	enum class ContextCreateFlag
	{
		withModuleResourceManage	             = 1 << 0,
		withModuleRenderGraph		             = 1 << 1,
        withAutoMaxFrameInFlight                 = 1 << 2,
		enableBindless                           = 1 << 3,
		enableRayTracing                         = 1 << 4,
		defaultConfig = withModuleResourceManage
						| withModuleRenderGraph
                        | withAutoMaxFrameInFlight
                        | enableBindless
                        | enableRayTracing,
	};
	FCT_DECLARE_FLAGS(ContextCreateFlag)
	class Context : public RefCount,public IEventSystem<EventSystemConfig::TriggerOnly>
	{
	protected:
		Runtime* m_runtime;
	protected:
		Context(Runtime* runtime);
		virtual ~Context();
		virtual void createPlatform(ContextCreateFlags flag) = 0;
	public:
        virtual bool isIntegrateGpu() = 0;
        virtual bool isRayTracingSupported() const noexcept = 0;
        virtual bool isMeshShaderSupported() const noexcept = 0;
        virtual bool isComputeSupported() const noexcept = 0;
        virtual bool isBindlessSupported() const noexcept = 0;
        virtual bool isReadWriteStorageBufferSupported() const noexcept = 0;
        virtual bool isIndirectDrawSupported() const noexcept = 0;
        virtual bool isBaseInstanceSupported() const noexcept = 0;
        virtual uint32_t getUniformBufferOffsetAlignment() const noexcept = 0;
        virtual uint32_t getMaxUniformBufferRange() const noexcept = 0;
        virtual bool isAccelerationStructureSupported() const noexcept { return isRayTracingSupported(); }
		PipeHub& pipeHub();
		/** @name 初始化与配置 (Initialization & Configuration)
   		*  @{
   		*/
		void create(ContextCreateFlags flag = ContextCreateFlag::defaultConfig);
		/** @} */
		/** @name 模块管理 (Module Management)
		 *  @{
		 */
		template <typename T>
		void addModule();
		template <typename T>
		void removeModule();
		template <typename T>
		T* getModule();
		/** @} */
		// tools
		template <typename T>
		T* createResource();
		virtual TraditionalPipeline* createTraditionalPipeline();
		virtual MeshShaderPipeline* createMeshShaderPipeline();
		virtual RayTracingPipeline* createRayTracingPipeline();
		virtual UniquePtr<RHI::Pipeline> createRhiPipeline(PipelineType type);
		[[deprecated("请迁移至 createTraditionalPipeline()")]]
		virtual RHI::RasterizationPipeline* createTraditionPipeline() = 0;
		virtual RHI::StorageBuffer* createStorageBuffer(size_t size = 0);
		virtual RHI::TransformFeedback* createTransformFeedback();
		virtual RHI::ConstBuffer* getEmptyConstBuffer(const ConstLayout& layout) = 0;
        virtual Sampler* getEmptySampler() = 0;
		StaticMesh<uint32_t>* createMesh(const ModelMesh* modelMesh, const VertexLayout& layout);
		StaticMesh<uint32_t>* loadMesh(const std::string& filename,const std::string& meshName, const VertexLayout& layout);
		Image* loadTexture(const std::string& filename);
		/**
		 * @brief 从转换到内存的文件中 读取
		 * @param data
		 * @param size
		 * @return
		 */
		Image* loadTexture(const unsigned char* data, size_t size);
	    /**
	     * @brief 加载图片并使用指定的Format
	     */
		Image* loadTexture(std::vector<unsigned char> data,Format dstFormat);
	    Image* loadTexture(const std::string& filename, Format dstFormat);
        Image* loadCubeMap(const std::vector<std::string>& filenames);
        
        virtual void acquireImageOwnership(Image* image) {} // For async transfer ownership transition
        /**
        /**
          * @cond CHINESE
          * @name 流程控制
          * @endcond
          *
          * @cond ENGLISH
          * @name Flow Control
          * @endcond
          *  @{
          */
		auto& syncTickers() { return m_flowControl->syncTickers(); }
		/*
		 * 初始化阶段 可以在flush前任意修改，因为提交线程一直在等待下一帧
		 * 运行阶段 不允许修改 或 拆分flush函数，在wait currentFlush和nextFrame之间修改
		 */
		auto& submitTickers() { return m_flowControl->submitTickers(); }
		FlowControl* getFlowControl() const noexcept { return m_flowControl; }
		void flush();
        void _consumeLogicTask()
        {
            ZoneScopedN("ConsumeLogicTasks");
            m_logicTask.consume_all([](LogicTaskData*& data) {
                data->task();
                FCT_DELETE(data);
            });
        }
        /** @} */
	protected:
		struct LogicTaskData
		{
			std::function<void()> task;
		};
		boost::lockfree::queue<LogicTaskData*,boost::lockfree::capacity<1024>> m_logicTask;
		void postLogicTask(const std::function<void()>& task)
		{
			LogicTaskData* data = FCT_NEW(LogicTaskData);
			data->task = task;
			m_logicTask.push(data);
		}
	public:

		/** @name 窗口管理 (Window Management)
		 *  @{
		 */
		const std::vector<Window*>& getBindWindows() { return m_bindWindows; }
		void onWindowBound(Window* wnd) {
			m_bindWindows.push_back(wnd);
			trigger(ContextEvent::WindowBound{wnd, this});
		}
		virtual void unbindWindow(Window* wnd);
		/** @} */
	public:
		void submitThread();
		void stopSubmitThread()
		{
			if (!m_ctxRunning.load(std::memory_order_relaxed))
			{
				return;
			}
			m_ctxRunning.store(false, std::memory_order_release);
			m_currentFlush.store(true, std::memory_order_release);
			m_nextFrame.store(true, std::memory_order_release);
			if (m_submitThread.joinable())
			{
				m_submitThread.join();
			}
		}
		/** @name 退出与资源销毁 (Shutdown & Resource Cleanup)
		 *  @{
		 */
		/// @brief 请求退出流程：先跳出主提交/渲染循环，然后等待 GPU 完成所有在飞任务，最后清理 GPU 资源
		void requestExit();
		virtual void waitGpu();
		virtual void cleanupGpuResources();
		/** @} */
		/** @name 着色器编译 (Shader Compilation)
  		  *  @{
  		  */
		void createCompiler();
		ShaderCompiler* getCompiler() { return m_compiler; }
		ShaderGenerator* getGenerator() { return m_generator; }
		/** @} */
	public:
		/** @name SubmitThread内部实现接口
	      *  @brief 以_开头的函数供自定义或内部使用
	      *  @{
	      */
		void _nextFrame();
		void _currentFlush();
		inline void _waitCurrentFlush()
		{
		    ZoneScopedN("WaitSubmitFlush");
			FCT_WAIT_FOR_WITH_TERM(m_currentFlush, !m_ctxRunning.load(std::memory_order_acquire));
		}
		inline void _waitForNextFrame()
		{
			FCT_WAIT_FOR_WITH_TERM(m_nextFrame, !m_ctxRunning.load(std::memory_order_acquire));
		}
		void advanceLogicFrame()
		{
			const uint32_t maxFrames = (m_maxFrameInFlight == 0) ? 1u : m_maxFrameInFlight;
			m_logicFrameIndex = (m_logicFrameIndex + 1) % maxFrames;
		}
		void advanceSubmitFrame()
		{
			const uint32_t maxFrames = (m_maxFrameInFlight == 0) ? 1u : m_maxFrameInFlight;
			m_submitFrameIndex = (m_submitFrameIndex + 1) % maxFrames;
		}
		/** @} */
	protected:
		// 模块组件
		FlowControl* m_flowControl = nullptr;
		Device* m_resourceDevice = nullptr;
		ModelLoader* m_modelLoader = nullptr;
		RenderGraph* m_renderGraph = nullptr;
		ImageLoader* m_imageLoader = nullptr;
		ResourceManager* m_resourceManager = nullptr;
		ShaderCompiler* m_compiler = nullptr;
		ShaderGenerator* m_generator = nullptr;
		CommandBufferGraph* m_cmdGraph = nullptr;
	protected:
		/*
		TokenGraph<std::string, SubmitTicker>& m_submitTickers;
		TokenGraph<std::string, SyncTicker>& m_syncTickers;
		*/
		SubmitTicker m_ticker;
		std::vector<Window*> m_bindWindows;
		std::atomic<bool> m_nextFrame{false};
		std::atomic<bool> m_currentFlush{true};
		std::thread m_submitThread;
		std::atomic<bool> m_ctxRunning{true};
		std::atomic<bool> m_exitRequested{false};
		std::atomic<bool> m_gpuCleanedUp{false};
	protected:
		std::thread m_transferThread;
		std::atomic<bool> m_transferRunning{false};
		
		virtual void transferThreadLoop() {}

	protected:
		uint32_t m_maxFrameInFlight{3};
		//std::map<Window*, RHI::DescriptorPool*> m_descriptorPools;
		RHI::DescriptorPool* m_descriptorPool = nullptr;
		size_t m_frameIndex = 0;//submit帧index 区别在于是 swapBuffer更改的，而m_submitFrameIndex和m_logicFrameIndex都是在同步时候更改的
		size_t m_logicFrameIndex = 0; //逻辑帧index
		size_t m_submitFrameIndex = 0;//submit帧index
		std::thread::id m_submitThreadId;
		//todo:未分离线程时，m_submitThreadId = 逻辑and提交线程 所在id
	public:
		/**
		 * @cond CHINESE
		 * @param maxFrameInFlight cpu可以比gpu快多少帧
		 * @note 必须在渲染之前调用
		 * @endcond
		 *
		 * @cond ENGLISH
		 * @param maxFrameInFlight Maximum number of frames in flight
		 * @note Must be called before rendering
		 * @endcond
		 */
		void maxFrameInFlight(uint32_t maxFrameInFlight);
		uint32_t maxFrameInFlight() const { return m_maxFrameInFlight; }
		uint32_t currentSubmitFrameIndex() const { return m_frameIndex; }
		/*
		 *分离了submit线程
		 *逻辑线程返回逻辑帧index
		 *提交线程返回提交帧index
		 *未分离submit 线程
		 *逻辑部分返回提交帧 index
		 *提交帧部分返返回提交帧 index
		 */
		uint32_t currentFrameIndex()
		{
			if (m_submitThreadId == std::this_thread::get_id())
			{
				return m_frameIndex;
			}
			return m_logicFrameIndex;
		}
		RHI::DescriptorPool* getDescriptorPool();
		//todo: 考虑可能要变更为 [IRenderTarget*]<->[DescriptorPool*] map
		uint32_t currentLogicFrameIndex() const { return m_logicFrameIndex; }
		void deferRelease(RefCount* resource);
		void flushDeferredReleases(uint32_t frameIdx);
		void flushAllDeferredReleases();
	protected:
		std::mutex m_deferredReleaseMutex;
		std::vector<std::vector<RefCount*>> m_deferredReleases;
	protected:
	public:
	protected:
	protected:
	public:
	};


	template <typename T>
	void Context::addModule()
	{
		if constexpr (std::is_same_v<T, ResourceManager>)
		{
			FCT_SAFE_NEW(m_resourceManager,ResourceManager,this);
		}
		else if constexpr (std::is_same_v<T, RenderGraph>)
		{
			FCT_SAFE_NEW(m_renderGraph,RenderGraph,pipeHub(),m_resourceDevice,m_flowControl,m_cmdGraph,m_resourceManager);
		}
		else
		{
			ferr << "Unsupported module type: " << typeid(T).name() << std::endl;
		}
	}

	template <typename T>
	void Context::removeModule()
	{
		if constexpr (std::is_same_v<T, ResourceManager>)
        {
			FCT_SAFE_DELETE(m_resourceManager);
        } else
        {
            ferr << "Unsupported module type: " << typeid(T).name() << std::endl;
        }

	}

	template <typename T>
	T* Context::getModule()
	{

		if constexpr (std::is_same_v<T, ResourceManager>)
		{
			return m_resourceManager;
		}
		else if constexpr (std::is_same_v<T, Device>)
		{
			return m_resourceDevice;
		}
		else if constexpr (std::is_same_v<T, RenderGraph>)
		{
            return m_renderGraph;
        }
		else if constexpr (std::is_same_v<T, CommandBufferGraph>)
		{
			return m_cmdGraph;
		}
		else
		{
			ferr << "try to get undefined context module." << std::endl;
			return nullptr;
		}
	}
}
#include "Mesh.hpp"
#include "Context.hpp"
