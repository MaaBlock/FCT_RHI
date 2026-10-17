//
// Created by Administrator on 2025/3/16.
//
#include "../FCTAPI.h"
#include "./Context.h"
#include "RenderGraph.h"
#include "../Runtime/runtime.h"


namespace FCT {
    void Context::_nextFrame() {
        m_nextFrame = true;
    }

    void Context::_currentFlush() {
        m_currentFlush = true;
    }

    Image* Context::loadTexture(const std::string& filename)
    {
        ZoneScoped();
        auto data = m_imageLoader->load(filename);
        SingleBufferImage* image = new SingleBufferImage(this);
        image->width(data.width);
        image->height(data.height);

        bool isSRGB = true;

        Format format;
        switch (data.channels) {
        case 1:
            format = Format::R8_UNORM;
            break;
        case 2:
            format = Format::R8G8_UNORM;
            break;
        case 3:
            format = isSRGB ? Format::R8G8B8A8_SRGB : Format::R8G8B8_UNORM;
            break;
        case 4:
            format = isSRGB ? Format::R8G8B8A8_SRGB : Format::R8G8B8A8_UNORM;
            break;
        default:
            ferr << "Unsupported number of channels: " << data.channels << std::endl;
            format = isSRGB ? Format::R8G8B8A8_SRGB : Format::R8G8B8A8_UNORM;
            break;
        }

        image->format(format);
        image->as(ImageUsage::Texture);
        image->initData(data.data.data(), data.data.size());
        image->create();
        return image;
    }

    Image* Context::loadTexture(const unsigned char* memData, size_t size)
    {
        ZoneScoped();
        auto data = m_imageLoader->loadFromMemory(memData, size);
        SingleBufferImage* image = new SingleBufferImage(this);
        image->width(data.width);
        image->height(data.height);

        bool isSRGB = true;

        Format format;
        switch (data.channels)
        {
        case 1:
            format = Format::R8_UNORM;
            break;
        case 2:
            format = Format::R8G8_UNORM;
            break;
        case 3:
            format = isSRGB ? Format::R8G8B8A8_SRGB : Format::R8G8B8_UNORM;
            break;
        case 4:
            format = isSRGB ? Format::R8G8B8A8_SRGB : Format::R8G8B8A8_UNORM;
            break;
        default:
            ferr << "Unsupported number of channels: " << data.channels << std::endl;
            format = isSRGB ? Format::R8G8B8A8_SRGB : Format::R8G8B8A8_UNORM;
            break;
        }

        image->format(format);
        image->as(ImageUsage::Texture);
        image->initData(data.data.data(), data.data.size());
        image->create();
        return image;
    }
    Image* Context::loadTexture(std::vector<unsigned char> byteData, Format dstFormat)
    {
        ZoneScoped();
        auto data = m_imageLoader->loadFromMemory(byteData.data(),byteData.size());
        SingleBufferImage* image = new SingleBufferImage(this);
        image->width(data.width);
        image->height(data.height);
        image->format(dstFormat);
        image->as(ImageUsage::Texture);
        image->initData(data.data.data(), data.data.size());
        image->create();
        return image;
    }
    Image* Context::loadTexture(const std::string& filename, Format dstFormat)
    {
        ZoneScoped();
        auto data = m_imageLoader->load(filename);
        SingleBufferImage* image = new SingleBufferImage(this);
        image->width(data.width);
        image->height(data.height);
        image->format(dstFormat);
        image->as(ImageUsage::Texture);
        image->initData(data.data.data(), data.data.size());
        image->create();
        return image;
    }

    Image* Context::loadCubeMap(const std::vector<std::string>& filenames)
    {
        ZoneScoped();
        if (filenames.size() != 6) {
            ferr << "loadCubeMap: Expected 6 filenames, got " << filenames.size() << std::endl;
            return nullptr;
        }

        std::vector<unsigned char> combinedData;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
        
        for (size_t i = 0; i < 6; ++i) {
            auto data = m_imageLoader->load(filenames[i]);
            if (data.data.empty()) {
                ferr << "loadCubeMap: Failed to load " << filenames[i] << std::endl;
                return nullptr;
            }

            if (i == 0) {
                width = data.width;
                height = data.height;
                channels = data.channels;
                combinedData.reserve(data.data.size() * 6);
            } else {
                if (data.width != width || data.height != height || data.channels != channels) {
                    ferr << "loadCubeMap: Image dimensions or channels mismatch in " << filenames[i] << std::endl;
                    return nullptr;
                }
            }
            combinedData.insert(combinedData.end(), data.data.begin(), data.data.end());
        }

        SingleBufferImage* image = new SingleBufferImage(this);
        image->width(width);
        image->height(height);
        image->arrayLayers(6);
        image->isCubeMap(true);

        bool isSRGB = true;
        Format format;
        switch (channels) {
            case 1: format = Format::R8_UNORM; break;
            case 2: format = Format::R8G8_UNORM; break;
            case 3: format = isSRGB ? Format::R8G8B8A8_SRGB : Format::R8G8B8_UNORM; break;
            case 4: format = isSRGB ? Format::R8G8B8A8_SRGB : Format::R8G8B8A8_UNORM; break;
            default: 
                format = Format::R8G8B8A8_SRGB; 
                ferr << "Unsupported channels: " << channels << std::endl;
                break;
        }

        image->format(format);
        image->as(ImageUsage::Texture);
        image->initData(combinedData.data(), combinedData.size());
        image->create();
        
        return image;
    }


    Context::Context(Runtime* runtime)
    {
        ZoneScoped();
        m_runtime = runtime;
        m_flowControl = new FlowControl();
        auto& m_submitTickers = m_flowControl->submitTickers();
        auto& m_syncTickers = m_flowControl->syncTickers();

        m_cmdGraph = nullptr;
        m_renderGraph = nullptr;
        m_resourceManager = nullptr;
        m_compiler = nullptr;
        m_ctxRunning = true;
        //m_flushWnd = nullptr;
        m_nextFrame = false;
        m_currentFlush = true;
        bool isInited = false;
        m_submitThread = std::thread([this,&isInited]
        {
            tracy::SetThreadName("RHI Submit Thread");
            m_submitThreadId = std::this_thread::get_id();
            isInited = true;
            submitThread();
        });
        FCT_WAIT_FOR(isInited);
        createCompiler();
        m_generator = new ShaderGenerator();
        m_frameIndex = 0;
        m_maxFrameInFlight = 3;
        m_imageLoader = runtime->createImageLoader();
        m_modelLoader = runtime->createModelLoader();
        m_submitTickers[SwapBufferSubmitTicker] = {
            [this]()
            {
                std::this_thread::yield();
                {
                    ZoneScopedN("SwapBuffer / WaitGPU");
                    ScopeTimer waitGpuTimer("waitGpu");
                    m_cmdGraph->swapBuffer();
                    //swapBuffers();
                }
            },
            {},
            {}
        };
        m_submitTickers.update();
        m_syncTickers[InnerSync::CheckRecreateSwapchainSync] =
            {
            [this]()
            {
                ZoneScopedN("CheckRecreateSwapchainSync");
                for (auto& window : m_bindWindows)
                {
                    window->swapchain()->sync();
                }
            },
            {},
            {InnerSync::AdvanceFrameIndex}
            };
        m_syncTickers[InnerSync::AdvanceFrameIndex] =
            {
            [this]()
            {
                ZoneScopedN("AdvanceFrameIndex");
                advanceLogicFrame();
                advanceSubmitFrame();
            },
            {},
            {}
            };
        m_syncTickers.update();
    }

    Context::~Context() {
        m_ctxRunning = false;
        FCT_SAFE_DELETE(m_renderGraph);
        m_submitThread.join();
    }

    PipeHub& Context::pipeHub()
    {
        return m_runtime->pipeHub();
    }

    void Context::create(ContextCreateFlags flag)
    {
        ZoneScoped();
        createPlatform(flag);
        m_descriptorPool = createResource<RHI::DescriptorPool>();
        m_descriptorPool->create();
        m_cmdGraph = new CommandBufferGraph(m_resourceDevice);
        if (flag & ContextCreateFlag::withModuleResourceManage)
            addModule<ResourceManager>();
        if (flag & ContextCreateFlag::withModuleRenderGraph)
            addModule<RenderGraph>();
        if (flag & ContextCreateFlag::withAutoMaxFrameInFlight) {
            if (isIntegrateGpu())
                maxFrameInFlight(1);
            else
                maxFrameInFlight(3);
        }
    }

    StaticMesh<uint32_t>* Context::createMesh(const ModelMesh* modelMesh, const VertexLayout& layout)
    {
        ZoneScoped();
        if (!modelMesh) {
            return nullptr;
        }

        StaticMesh<uint32_t>* mesh = new StaticMesh<uint32_t>(this, layout);
        VertexBuffer* vertexBuffer = mesh->getVertexBuffer();
        vertexBuffer->resize(modelMesh->vertices.size());
        for (uint32_t i = 0; i < modelMesh->vertices.size(); ++i) {
            const auto& modelVertex = modelMesh->vertices[i];
            Vertex vertex = (*vertexBuffer)[i];

            for (size_t j = 0; j < layout.getElementCount(); ++j) {
                const VertexElement& element = layout.getElement(j);
                setVertexAttributeFromModel(vertex, j, element, modelVertex);
            }
        }

        std::vector<uint32_t> indices = modelMesh->indices;
        mesh->setIndices(indices);

        mesh->create();

        return mesh;
    }

    StaticMesh<uint32_t>* Context::loadMesh(const std::string& filename, const std::string& meshName,
                                            const VertexLayout& layout)
    {
        ZoneScoped();
        auto md = m_modelLoader->loadModel(filename);
        auto mMesh =  md->findMesh(meshName);
        return createMesh(mMesh,layout);
    }
}
