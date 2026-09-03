//
// Created by Administrator on 2025/7/28.
//
#include "ResourceManager.h"

#include "MutilBufferImage.h"
#include "SingleBufferImage.h"

#include "./Context.h"
#include "../Base/TokenGraph.h"
#include "../RHI/Swapcain.h"
#include "../UI/Window.h"

namespace FCT {
    std::string WindowToToken(Window* wnd)
    {
        return SwapchainToToken(wnd->swapchain());
    }

    std::string SwapchainToToken(RHI::Swapchain* swapchain)
    {
        return "Swapchain_" + std::to_string(reinterpret_cast<uintptr_t>(swapchain));
    }

    ImageSaved::ImageSaved(RHI::Swapchain* swapchain)
    {
        img = swapchain->image();
        autoIndex = false;
    }

    ImageSaved::ImageSaved(ImageSaved* denpendency)
    {
        mutilBuffer = denpendency->mutilBuffer;
        autoIndex = true;
    }

    ResourceManager::ResourceManager(Context* ctx)
    {
        m_resourceDevice = ctx->getModule<Device>();
        m_context = ctx;
        m_context->subscribe<ContextEvent::WindowBound>([this](const ContextEvent::WindowBound& env)
        {
            registerWindow(env.window);
        });
        auto& tickerGraph = m_context->submitTickers();
        tickerGraph[InnerTicker::ImageGraph_ChangeIndex] = {
            [this]()
            {
                for (auto image : m_needChangeIndexImages)
                {
                    auto img = dynamic_cast<MutilBufferImage*>(image.img);
                    img->changeCurrentIndex(m_context->currentLogicFrameIndex());
                }
            },
            {SwapBufferSubmitTicker},
            {}
        };
        for (auto wnd : m_context->getBindWindows())
        {
            registerWindow(wnd);
        }
    }

    Image* ResourceManager::allocateImage(std::string name, Window* dependency,ImageDesc desc)
    {
        return allocateImage(name, WindowToToken(dependency),desc);
    }

    Image* ResourceManager::allocateImage(std::string name, std::string dependency,ImageDesc desc)
    {
        auto saved = m_dependencyGraph[dependency]->value;
        auto ret = ImageSaved(saved);
        if (ret.mutilBuffer)
        {
            auto savedImg = dynamic_cast<MutilBufferImage*>(saved.img);
            auto img = m_resourceDevice->createResource<MutilBufferImage>();
            img->samples(savedImg->samples());
            img->format(desc.format);
            img->width(savedImg->width());
            img->height(savedImg->height());
            img->imageCount(savedImg->imageCount());
            img->as(desc.usage);
            img->create();
            img->changeCurrentIndex(m_context->currentLogicFrameIndex());
            ret.img = img;
        }  else
        {
            auto savedImg = dynamic_cast<SingleBufferImage*>(saved.img);
            auto img = m_resourceDevice->createResource<SingleBufferImage>();
            img->samples(savedImg->samples());
            img->format(desc.format);
            img->width(savedImg->width());
            img->height(savedImg->height());
            img->as(desc.usage);
            img->create();
            ret.img = img;
        }
        m_dependencyGraph[name] = {
            ret,
            {dependency},
            {},
        };
        updateGraph();
        return ret.img;
    }

    void ResourceManager::clearImage(std::string name)
    {
        if (m_dependencyGraph.containsNode(name))
        {
            {
                auto node = m_dependencyGraph[name];
                auto& imageSaved = node->value;

                if (imageSaved.img)
                {
                    imageSaved.img->release();
                }
            }

            m_dependencyGraph.removeNode(name);

            updateGraph();
        }
    }

    void ResourceManager::updateGraph()
    {
        auto status = m_dependencyGraph.update();
        if (!status.ok()) {
            return;
        }
        auto resultOr = m_dependencyGraph.computeOrder([](const ImageSaved& saved)
        {
            return saved.mutilBuffer && saved.autoIndex;
        });
        if (resultOr.ok()) {
            m_needChangeIndexImages = std::move(*resultOr);
        }
    }

    void ResourceManager::registerWindow(Window* wnd)
    {
        std::string token = WindowToToken(wnd);
        RHI::Swapchain* swapchain = wnd->swapchain();
        swapchain->subscribe<SwapchainEvent::Recreate>([this](const SwapchainEvent::Recreate& env)
        {
            resizeSub(
                SwapchainToToken(env.swapchain),
                env.width,
                env.height);
        });
        m_dependencyGraph[token] = {
            ImageSaved(swapchain),
            {},
            {},
        };
        updateGraph();
    }

    void ResourceManager::resizeSub(std::string token, int width, int height)
    {
        m_dependencyGraph.visitBFS(token,[width,height](ImageSaved img)
        {
            img.img->resize(width,height);
        },false);
    }

    void ResourceManager::resize(std::string token, int width, int height)
    {
        m_dependencyGraph.visitBFS(token,[width,height](ImageSaved img)
        {
            img.img->resize(width,height);
        },true);
    }


}
//todo:
namespace FCT
{
    void ResourceManager::resizeImage(std::string name, int width, int height)
    {

    }
    Image* ResourceManager::allocateTarget(std::string name, TargetDesc desc)
    {
        auto ret = ImageSaved();
        auto img = m_resourceDevice->createResource<MutilBufferImage>();
        img->samples(desc.samples);
        img->format(desc.format);
        img->width(desc.width);
        img->height(desc.height);
        img->imageCount(m_context->maxFrameInFlight());
        img->as(desc.usage);
        img->create();
        img->changeCurrentIndex(m_context->currentLogicFrameIndex());
        ret.img = img;
        m_dependencyGraph[name] = {
            ret,
            {},
            {},
        };
        updateGraph();
        return ret.img;
    }
}