#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    struct SwapchainData
    {
        vk::SurfaceKHR surface{};
        vk::SwapchainKHR swapchain{};
        std::uint32_t imageIndex{};
    };

    auto internal_swapchain_create(const SwapchainInfo& swapchainInfo) -> std::expected<vk::SwapchainKHR, ResultCode>;
    void internal_swapchain_destroy(vk::SwapchainKHR swapchain);

    auto internal_swapchain_get(vk::SwapchainKHR swapchain) -> std::expected<std::reference_wrapper<SwapchainData>, ResultCode>;

    auto internal_swapchain_acquire_next_image(const AcquireInfo& acquireInfo) -> std::expected<std::uint32_t, ResultCode>;
    auto internal_swapchain_present(const PresentInfo& presentInfo) -> ResultCode;

}