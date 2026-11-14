//
// Created by Administrator on 2025/7/28.
//

#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H
#include "../ThirdParty.h"
#include "ContextEvent.h"
#include "../Base/TokenGraph.h"
#include "Device.h"
#include "FlowControl.h"
#include "Format.h"
#include "../RHI/ImageUsageFlags.h"
#include "../Base/FractionScale2D.h"
#include "ResourceGroup.h"
#include "../MutilThreadBase/UniquePtr.h"

namespace FCT
{
    class Image;
    class Buffer;
    class MutilBufferImage;
    class SingleBufferImage;
    class Context;
    class Window;
    namespace RHI
    {
        class Swapchain;
    }

    std::string WindowToToken(Window* wnd);
    std::string SwapchainToToken(RHI::Swapchain* swapchain);
    struct ImageDesc
    {
        Format format;
        Samples samples;
        ImageUsages usage;
    };
    struct TargetDesc
    {
        int width;
        int height;
        Samples samples;
        Format format;
        ImageUsages usage;
    };
    struct ImageSaved
    {
        ImageSaved(RHI::Swapchain* swapchain);
        ImageSaved(ImageSaved* denpendency);
        ImageSaved()
        {
            mutilBuffer = true;
            autoIndex = true;
        }
        Image* img;
        bool mutilBuffer;
        bool autoIndex;
    };

    /**
     * @cond CHINESE
     * @note  只负责Image的自动Resize和
     *        自动切换BufferIndex
     * @endcond
     */
    namespace InnerTicker
    {
        constexpr const char* ImageGraph_ChangeIndex = "ImageGraph_ChangeIndex";
    }
    class ResourceManager {
    public:
        ResourceManager(Context* ctx);
        /**
         * @cond CHINESE
         * @brief 分配中间Target / 图像
         * @param name
         * @param dependency
         * @return
         * @endcond
         */
        Image* allocateImage(std::string name,Window* dependency,ImageDesc desc);
        /**
         * @cond CHINESE
         * @brief 分配中间Target / 图像
         * @param name
         * @param dependency
         * @return
         * @endcond
         */
        Image* allocateImage(std::string name,std::string dependency,ImageDesc desc);
        void clearImage(std::string name);
    private:
        void updateGraph();
        void addvanceFrame();
        void registerWindow(Window* wnd);
        void resizeSub(std::string token,int width, int height);
        void resize(std::string token, int width, int height);
        TokenGraph<std::string,ImageSaved> m_dependencyGraph;
        std::vector<std::string> m_target;
        std::vector<ImageSaved> m_needChangeIndexImages;
        Device* m_resourceDevice;
        Context* m_context;
        FlowControl* m_flowControl;
        std::unordered_map<std::string, UniquePtr<ResourceGroup>> m_resourceGroups;
    public:
        void resizeImage(std::string name, int width, int height);
        Image* allocateTarget(std::string name,TargetDesc desc);
        Image* registerExternalTarget(std::string name, Image* externalImg);
        Image* require(const std::string& groupName, const ResourceRequireDesc& desc);
        Buffer* requireBuffer(const std::string& groupName, const BufferRequireDesc& desc);
        ResourceGroup* getOrCreateResourceGroup(const std::string& groupName);
        ResourceGroup* getResourceGroup(const std::string& groupName) const;
        void releaseGroupResource(const std::string& groupName, const std::string& resourceId);
        void releaseGroupBuffer(const std::string& groupName, const std::string& resourceId);
        std::vector<ResourceGroup*> getAllResourceGroups() const;
    };

}

#endif //RESOURCEMANAGER_H
