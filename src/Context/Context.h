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
#include "./OldRenderGraph.h"
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
	class OldPass;
	namespace RHI
	{
		class TextureView;
		class ConstBuffer;
		class IndexBuffer;
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
		defaultConfig = withModuleResourceManage
						| withModuleRenderGraph
                        | withAutoMaxFrameInFlight,
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
        virtual bool isBindlessSupported() const = 0;
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
		virtual RHI::RasterizationPipeline* createTraditionPipeline() = 0;
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
		void flush();
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
		/** @} */
	public:
		void submitThread();
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
		constexpr inline void _waitCurrentFlush()
		{
			FCT_WAIT_FOR(m_currentFlush);
		}
		constexpr inline void _waitForNextFrame()
		{
			FCT_WAIT_FOR_WITH_TERM(m_nextFrame,!m_ctxRunning);
		}
		void advanceLogicFrame()
		{
			m_logicFrameIndex = (m_logicFrameIndex + 1) % m_maxFrameInFlight;
		}
		void advanceSubmitFrame()
		{
			m_submitFrameIndex = (m_submitFrameIndex + 1) % m_maxFrameInFlight;
		}
		/** @} */
	protected:
		// 模块组件
		FlowControl* m_flowControl;
		Device* m_resourceDevice;
		ModelLoader* m_modelLoader;
		RenderGraph* m_renderGraph;
		ImageLoader* m_imageLoader;
		ResourceManager* m_resourceManager;
		ShaderCompiler* m_compiler;
		ShaderGenerator* m_generator;
		CommandBufferGraph* m_cmdGraph;
	protected:
		/*
		TokenGraph<std::string, SubmitTicker>& m_submitTickers;
		TokenGraph<std::string, SyncTicker>& m_syncTickers;
		*/
		SubmitTicker m_ticker;
		std::vector<Window*> m_bindWindows;
		bool m_nextFrame;
		bool m_currentFlush;
		std::thread m_submitThread;
		bool m_ctxRunning;
	protected:
		std::thread m_transferThread;
		std::atomic<bool> m_transferRunning;
		
		virtual void transferThreadLoop() {}

	protected:
		uint32_t m_maxFrameInFlight;
		//std::map<Window*, RHI::DescriptorPool*> m_descriptorPools;
		RHI::DescriptorPool* m_descriptorPool;
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
