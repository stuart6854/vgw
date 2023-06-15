#pragma once

#include "base.hpp"
#include "device.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <functional>
#include <string_view>
#include <source_location>

namespace VGW_NAMESPACE
{
    class Device;

    enum class LogLevel : std::uint8_t
    {
        eDebug = 0,
        eInfo = 1,
        eWarn = 2,
        eError = 3,
    };

    using LogCallbackFunc = std::function<void(LogLevel logLevel, const std::string_view& msg, const std::source_location& sourceLocation)>;

    class Context
    {
    public:
        Context() = default;
        explicit Context(const ContextInfo& contextInfo);
        Context(const Context&) = delete;
        Context(Context&& other) noexcept;
        ~Context();

        /* Getters */

        /**
         * Is this a valid/initialised instance?
         */
        bool is_valid() const;

        auto get_instance() const -> vk::Instance { return m_instance; }

        /* Methods */

        void destroy();

        /**
         * Set the log level. Only logs => the log level are output.
         * @param logLevel The minimum log level to output.
         */
        void set_log_level(LogLevel logLevel);

        /**
         * Set the function to output logs to.
         * @param logCallbackFunc Function to use to output logs.
         */
        void set_log_callback(LogCallbackFunc&& logCallbackFunc);

        void log(LogLevel logLevel,
                 const std::string_view& msg,
                 const std::source_location& sourceLocation = std::source_location::current()) const;
        void log_debug(const std::string_view& msg, const std::source_location& sourceLocation = std::source_location::current()) const;
        void log_info(const std::string_view& msg, const std::source_location& sourceLocation = std::source_location::current()) const;
        void log_warn(const std::string_view& msg, const std::source_location& sourceLocation = std::source_location::current()) const;
        void log_error(const std::string_view& msg, const std::source_location& sourceLocation = std::source_location::current()) const;

        auto create_device(const vgw::DeviceInfo& deviceInfo) -> std::unique_ptr<Device>;

        auto windowHwnd(void* platformSurfaceHandle) const -> vk::UniqueSurfaceKHR;

        /* Operators */

        auto operator=(const Context&) -> Context& = delete;
        auto operator=(Context&& rhs) noexcept -> Context&;

    private:
        /* Checks if this instance is invariant. Used to check pre-/post-conditions for methods. */
        void is_invariant() const;

    private:
        LogLevel m_minLogLevel{ LogLevel::eWarn };
        LogCallbackFunc m_logCallbackFunc;

        vk::Instance m_instance;

        bool m_debugCallbackSupported{};
        vk::DebugUtilsMessengerCreateInfoEXT m_debugUtilsCallbackCreateInfo;
        vk::DebugUtilsMessengerEXT m_debugCallback;
    };

    auto create_context(const vgw::ContextInfo& contextInfo) -> std::unique_ptr<Context>;
}