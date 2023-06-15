#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

#include <vulkan/vulkan.hpp>

#include <expected>

namespace vgw::internal
{
    struct ContextData
    {
        vk::DynamicLoader loader{};
        vk::Instance instance{};
        vk::DebugUtilsMessengerEXT messenger{};

        ~ContextData();
    };

    auto internal_context_init(const ContextInfo& contextInfo) -> ResultCode;
    void internal_context_destroy();

    auto internal_context_get() -> std::expected<std::reference_wrapper<ContextData>, ResultCode>;

    bool internal_is_context_valid() noexcept;

}