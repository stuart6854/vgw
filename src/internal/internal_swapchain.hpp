#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    struct SwapchainData
    {
        vk::SurfaceKHR surface{};
        vk::SwapchainKHR swapchain{};
    };

    auto internal_swapchain_create(const SwapchainInfo& swapchainInfo) -> std::expected<vk::SwapchainKHR, ResultCode>;
    void internal_swapchain_destroy(vk::SwapchainKHR swapchain);

}