#include "vgw/context.hpp"

#include "context_helpers.hpp"

#include <ranges>
#include <format>
#include <iostream>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace VGW_NAMESPACE
{
    constexpr const char* VGW_VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";

    void DefaultLogCallback(vgw::LogLevel logLevel, const std::string_view& msg, const std::source_location& sourceLocation)
    {
        if (logLevel == vgw::LogLevel::eError)
        {
            auto formattedMsg = std::format("{}:{} | {}", sourceLocation.file_name(), sourceLocation.line(), msg);
            std::cerr << "[VGW-Error] " << formattedMsg << std::endl;
        }
        else if (logLevel == vgw::LogLevel::eWarn)
        {
            std::cout << "[VGW-Warn] " << msg << std::endl;
        }
        else if (logLevel == vgw::LogLevel::eInfo)
        {
            std::cout << "[VGW-Info] " << msg << std::endl;
        }
        else if (logLevel == vgw::LogLevel::eDebug)
        {
            std::cout << "[VGW-Debug] " << msg << std::endl;
        }
    }

    auto VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                             VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                             void* pUserData) -> VkBool32;

    Context::Context(const ContextInfo& contextInfo) : m_minLogLevel(LogLevel::eDebug), m_logCallbackFunc(DefaultLogCallback)
    {
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
                auto msg = std::format("Layer <{}> is not supported. It will not be enabled.", layer);
                log_warn(msg);
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
                auto msg = std::format("Extension <{}> is not supported. It will not be enabled.", extension);
                log_warn(msg);
            }
        }

        log_info("Enabled layers:");
        for (const auto& layer : enabledLayers)
        {
            auto msg = std::format("  {}", layer);
            log_info(msg);
        }

        log_info("Enabled extensions:");
        for (const auto& extension : enabledExtensions)
        {
            auto msg = std::format("  {}", extension);
            log_info(msg);
        }

#pragma endregion
        m_debugCallbackSupported =
            std::ranges::find(enabledExtensions, std::string_view(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) != enabledExtensions.end();

        m_debugUtilsCallbackCreateInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        m_debugUtilsCallbackCreateInfo.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
        m_debugUtilsCallbackCreateInfo.setPfnUserCallback(VulkanDebugCallback);
        m_debugUtilsCallbackCreateInfo.setPUserData(this);

        vk::InstanceCreateInfo instanceInfo{ vk::InstanceCreateFlags(), &appInfo, enabledLayers, enabledExtensions };
        if (m_debugCallbackSupported)
        {
            instanceInfo.setPNext(&m_debugUtilsCallbackCreateInfo);
        }

        m_instance = vk::createInstance(instanceInfo).value;
        if (!m_instance)
        {
            log_error("Failed to create vk::Instance!");
            return;
        }

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);

        if (m_debugCallbackSupported)
        {
            m_debugCallback = m_instance.createDebugUtilsMessengerEXT(m_debugUtilsCallbackCreateInfo).value;
            if (!m_debugCallback)
            {
                log_error("Failed to create Vulkan debug messenger!");
            }
        }

        is_invariant();
    }

    Context::Context(Context&& other) noexcept
    {
        std::swap(m_minLogLevel, other.m_minLogLevel);
        std::swap(m_logCallbackFunc, other.m_logCallbackFunc);
        std::swap(m_instance, other.m_instance);
        std::swap(m_debugCallbackSupported, other.m_debugCallbackSupported);
        std::swap(m_debugUtilsCallbackCreateInfo, other.m_debugUtilsCallbackCreateInfo);
        std::swap(m_debugCallback, other.m_debugCallback);

        if (m_debugCallbackSupported)
        {
            m_instance.destroy(m_debugCallback);
            m_debugUtilsCallbackCreateInfo.setPUserData(this);
            m_debugCallback = m_instance.createDebugUtilsMessengerEXT(m_debugUtilsCallbackCreateInfo).value;
            if (!m_debugCallback)
            {
                log_error("Failed to create Vulkan debug messenger!");
            }
        }
    }

    Context::~Context()
    {
        destroy();
    }

    bool Context::is_valid() const
    {
        return m_instance;
    }

    void Context::destroy()
    {
        if (is_valid())
        {
            m_instance.destroy(m_debugCallback);
            m_debugCallback = nullptr;
            m_instance.destroy();
            m_instance = nullptr;
        }
    }

    void Context::set_log_level(LogLevel logLevel)
    {
        m_minLogLevel = logLevel;
    }

    void Context::set_log_callback(LogCallbackFunc&& logCallbackFunc)
    {
        m_logCallbackFunc = logCallbackFunc;
    }

    void Context::log(LogLevel logLevel, const std::string_view& msg, const std::source_location& sourceLocation) const
    {
        if (logLevel >= m_minLogLevel)
        {
            m_logCallbackFunc(logLevel, msg, sourceLocation);
        }
    }

    void Context::log_debug(const std::string_view& msg, const std::source_location& sourceLocation) const
    {
        log(LogLevel::eDebug, msg, sourceLocation);
    }

    void Context::log_info(const std::string_view& msg, const std::source_location& sourceLocation) const
    {
        log(LogLevel::eInfo, msg, sourceLocation);
    }

    void Context::log_warn(const std::string_view& msg, const std::source_location& sourceLocation) const
    {
        log(LogLevel::eWarn, msg, sourceLocation);
    }

    void Context::log_error(const std::string_view& msg, const std::source_location& sourceLocation) const
    {
        log(LogLevel::eError, msg, sourceLocation);
    }

    auto Context::create_device(const vgw::DeviceInfo& deviceInfo) -> std::unique_ptr<Device>
    {
        return std::make_unique<Device>(*this, deviceInfo);
    }

    auto Context::windowHwnd(void* platformSurfaceHandle) const -> vk::UniqueSurfaceKHR
    {
#if _WIN32
        vk::Win32SurfaceCreateInfoKHR surfaceInfo{};
        surfaceInfo.setHinstance(GetModuleHandle(nullptr));
        surfaceInfo.setHwnd(static_cast<HWND>(platformSurfaceHandle));
        return m_instance.createWin32SurfaceKHRUnique(surfaceInfo).value;
#endif
    }

    void Context::is_invariant() const
    {
        VGW_ASSERT(m_instance);
    }

    auto Context::operator=(Context&& rhs) noexcept -> Context&
    {
        std::swap(m_minLogLevel, rhs.m_minLogLevel);
        std::swap(m_logCallbackFunc, rhs.m_logCallbackFunc);
        std::swap(m_instance, rhs.m_instance);
        std::swap(m_debugCallbackSupported, rhs.m_debugCallbackSupported);
        std::swap(m_debugUtilsCallbackCreateInfo, rhs.m_debugUtilsCallbackCreateInfo);
        std::swap(m_debugCallback, rhs.m_debugCallback);
        if (m_debugCallbackSupported)
        {
            m_instance.destroy(m_debugCallback);
            m_debugUtilsCallbackCreateInfo.setPUserData(this);
            m_debugCallback = m_instance.createDebugUtilsMessengerEXT(m_debugUtilsCallbackCreateInfo).value;
            if (!m_debugCallback)
            {
                log_error("Failed to create Vulkan debug messenger!");
            }
        }
        return *this;
    }

    auto VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                             VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                             void* pUserData) -> VkBool32
    {
        LogLevel logLevel{};
        switch (messageSeverity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: logLevel = LogLevel::eDebug; break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: logLevel = LogLevel::eInfo; break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: logLevel = LogLevel::eWarn; break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: logLevel = LogLevel::eError; break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
            default: break;
        }

        auto* context = static_cast<Context*>(pUserData);
        VGW_ASSERT(context && context->is_valid());
        context->log(logLevel, pCallbackData->pMessage);
        return false;
    }

    auto create_context(const vgw::ContextInfo& contextInfo) -> std::unique_ptr<Context>
    {
        return std::make_unique<Context>(contextInfo);
    }
}