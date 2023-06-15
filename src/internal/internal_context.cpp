#include "internal_context.hpp"

#include "../context_helpers.hpp"

#include <memory>
#include <iostream>

namespace VGW_NAMESPACE::internal
{
    constexpr const char* VGW_VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";

    auto VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                             VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                             void* pUserData) -> VkBool32;

    ContextData::~ContextData()
    {
        if (instance)
        {
            // #TODO: Throw error about not having destroyed the context
            internal_context_destroy();
        }
    }

    auto internal_context_init(const ContextInfo& contextInfo) -> ResultCode
    {
        auto& contextRef = internal_context_get().value().get();

        vk::DynamicLoader dl;
        auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        vk::ApplicationInfo appInfo{
            contextInfo.appName, contextInfo.appVersion, contextInfo.engineName, contextInfo.engineVersion, VK_API_VERSION_1_3
        };

#pragma region Layers & Extensions

        std::vector<const char*> wantedLayers;
        std::vector<const char*> wantedExtensions;
        if (contextInfo.enableDebug)
        {
            wantedLayers.push_back(VGW_VALIDATION_LAYER);
            wantedExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        if (contextInfo.enableSurfaces)
        {
            wantedExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
            wantedExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
        }

        std::vector<const char*> enabledLayers;
        std::vector<const char*> enabledExtensions;
        for (const auto& layer : wantedLayers)
        {
            if (is_instance_layer_supported(layer))
            {
                enabledLayers.push_back(layer);
            }
            else
            {
                log_warn("Instance layer <{}> is not supported. It will not be enabled!", layer);
            }
        }
        for (const auto& extension : wantedExtensions)
        {
            if (is_instance_extension_supported(extension))
            {
                enabledExtensions.push_back(extension);
            }
            else
            {
                log_warn("Instance extension <{}> is not supported. It will not be enabled!", extension);
            }
        }

        log_debug("Enabled layers:");
        for (const auto& layer : enabledLayers)
        {
            log_debug("  {}", layer);
        }

        log_debug("Enabled extensions:");
        for (const auto& extension : enabledExtensions)
        {
            log_info("  {}", extension);
        }

#pragma endregion
        bool debugCallbackSupported =
            std::ranges::find(enabledExtensions, std::string_view(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) != enabledExtensions.end();

        vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo{};
        messengerCreateInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        messengerCreateInfo.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
        messengerCreateInfo.setPfnUserCallback(VulkanDebugCallback);

        vk::InstanceCreateInfo instanceInfo{ vk::InstanceCreateFlags(), &appInfo, enabledLayers, enabledExtensions };
        if (debugCallbackSupported)
        {
            instanceInfo.setPNext(&messengerCreateInfo);
        }

        contextRef.instance = vk::createInstance(instanceInfo).value;
        if (!contextRef.instance)
        {
            log_error("Failed to create vk::Instance!");
            return ResultCode::eFailedToCreate;
        }

        VULKAN_HPP_DEFAULT_DISPATCHER.init(contextRef.instance);

        if (debugCallbackSupported)
        {
            contextRef.messenger = contextRef.instance.createDebugUtilsMessengerEXT(messengerCreateInfo).value;
            if (!contextRef.messenger)
            {
                log_error("Failed to create Vulkan debug messenger!");
            }
        }

        return ResultCode::eSuccess;
    }

    void internal_context_destroy()
    {
        auto& contextRef = internal_context_get().value().get();

        contextRef.instance.destroy(contextRef.messenger);
        contextRef.instance.destroy();
    }

    auto internal_context_get() -> std::expected<std::reference_wrapper<ContextData>, ResultCode>
    {
        static std::unique_ptr<ContextData> s_context{ nullptr };
        if (s_context == nullptr)
        {
            s_context = std::make_unique<ContextData>();
        }
        return std::ref(*s_context);
    }

    auto VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                             VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                             void* pUserData) -> VkBool32
    {
        MessageType msgType{};
        switch (messageSeverity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: msgType = MessageType::eDebug; break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: msgType = MessageType::eInfo; break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: msgType = MessageType::eWarning; break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: msgType = MessageType::eError; break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
            default: break;
        }

        log(msgType, pCallbackData->pMessage);
        return false;
    }
}