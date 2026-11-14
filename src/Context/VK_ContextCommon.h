//
// Created by Administrator on 2025/3/1.
//
#include "../ThirdParty.h"
#include "../DebugTools/OutStream.h"
#include "./VK_Context.h"
#ifndef FCT_VK_CONTEXTCOMMON_H
#define FCT_VK_CONTEXTCOMMON_H
namespace FCT {
    inline vk::DebugUtilsMessengerEXT g_vkDebugMessenger{nullptr};

    class VK_ContextCommon {
    public:
        VK_ContextCommon(Runtime* runtime) {
            m_runtime = runtime;
            m_pickDeviceId = 0;
        }
        void addLayer(const char* layer) {
            m_layers.push_back(layer);
        }
        void loaderVulkan()
        {
            //auto vkGetInstanceProcAddr = m_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
            //VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
        }
        void init() {
            loaderVulkan();
            VULKAN_HPP_DEFAULT_DISPATCHER.init();
            std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
            auto hasLayer = [&availableLayers](const char* name) {
                for (const auto& layer : availableLayers) {
                    if (std::strcmp(layer.layerName, name) == 0) {
                        return true;
                    }
                }
                return false;
            };
#ifdef FCT_DEBUG
            if (hasLayer("VK_LAYER_KHRONOS_validation")) {
                m_layers.push_back("VK_LAYER_KHRONOS_validation");
            }
#endif
            const char* envLayers = std::getenv("VK_INSTANCE_LAYERS");
            if (envLayers && std::strstr(envLayers, "VK_LAYER_KHRONOS_validation")) {
                if (hasLayer("VK_LAYER_KHRONOS_validation") &&
                    std::find_if(m_layers.begin(), m_layers.end(), [](const char* l){ return std::strcmp(l, "VK_LAYER_KHRONOS_validation") == 0; }) == m_layers.end()) {
                    m_layers.push_back("VK_LAYER_KHRONOS_validation");
                }
            }
            uint32_t apiVersion = VK_API_VERSION_1_0;
            auto pfnEnumerate = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
                vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
            if (pfnEnumerate) {
                uint32_t supportedApi = 0;
                if (pfnEnumerate(&supportedApi) == VK_SUCCESS) {
                    apiVersion = std::min(supportedApi, static_cast<uint32_t>(VK_API_VERSION_1_2));
                }
            }

            vk::ApplicationInfo appInfo;
            appInfo.setPApplicationName("Vulkan Application")
                   .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
                   .setPEngineName("FCT")
                   .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
                   .setApiVersion(apiVersion);
            std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
            auto hasExtension = [&availableExtensions](const char* name) {
                for (const auto& ext : availableExtensions) {
                    if (std::strcmp(ext.extensionName, name) == 0) {
                        return true;
                    }
                }
                return false;
            };

            // 输出可用的扩展，用于调试
            fout << "Available Vulkan extensions:" << std::endl;
            for (const auto& extension : availableExtensions) {
                fout << "  " << extension.extensionName << std::endl;
            }
            std::vector<const char*> extensions = {
#ifdef FCT_ANDROID
                VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#endif
#ifdef FCT_WIN32
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
                VK_KHR_SURFACE_EXTENSION_NAME,
            };

            bool enableDebugUtils = false;
            if (hasExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                enableDebugUtils = true;
            }

            vk::InstanceCreateInfo createInfo;
#ifdef FCT_DEBUG
            std::vector<vk::ValidationFeatureEnableEXT> enabledValidationFeatures;
            vk::ValidationFeaturesEXT validationFeatures;
            if (hasExtension(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME)) {
                extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
                enabledValidationFeatures.push_back(vk::ValidationFeatureEnableEXT::eSynchronizationValidation);
                validationFeatures.setEnabledValidationFeatures(enabledValidationFeatures);
                createInfo.setPNext(&validationFeatures);
            }
#endif
            createInfo.setPEnabledLayerNames(m_layers)
                      .setPApplicationInfo(&appInfo)
                      .setPEnabledExtensionNames(extensions);
            m_instance = vk::createInstance(createInfo);
            VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);

            if (enableDebugUtils) {
                vk::DebugUtilsMessengerCreateInfoEXT debugInfo{};
                debugInfo.setMessageSeverity(
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
                );
                debugInfo.setMessageType(
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                );
                debugInfo.setPfnUserCallback([](
                    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                    void* pUserData) -> VkBool32
                {
                    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                        ferr << "[Vulkan Validation Error] " << (pCallbackData->pMessage ? pCallbackData->pMessage : "") << std::endl;
                    } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                        ferr << "[Vulkan Validation Warning] " << (pCallbackData->pMessage ? pCallbackData->pMessage : "") << std::endl;
                    }
                    return VK_FALSE;
                });
                g_vkDebugMessenger = m_instance.createDebugUtilsMessengerEXT(debugInfo);
            }

            m_physicalDevices =  m_instance.enumeratePhysicalDevices();
            for (auto it : m_physicalDevices){
                fout << "Physical Device: " << it.getProperties().deviceName << std::endl;
                for (auto& queueFamily : it.getQueueFamilyProperties()) {
                    fout << "Queue Family: " << queueFamily.queueCount << std::endl;
                }
            }


        }

        void term() {
            if (g_vkDebugMessenger) {
                m_instance.destroyDebugUtilsMessengerEXT(g_vkDebugMessenger);
                g_vkDebugMessenger = nullptr;
            }
            m_instance.destroy();
        }
        VK_Context* createContext() {
            return new VK_Context(this);
        }
        auto getPhysicalDevice() {
            return m_physicalDevices[m_pickDeviceId];
        }
        auto getInstance() {
            return m_instance;
        }
        auto runtime()
        {
            return m_runtime;
        }
    private:
        size_t m_pickDeviceId;
        vk::Instance m_instance;
        std::vector<const char*> m_layers;
        std::vector<vk::PhysicalDevice> m_physicalDevices;
        vk::detail::DynamicLoader m_loader;
        Runtime* m_runtime;
    };
}
#endif //FCT_VK_CONTEXTCOMMON_H
