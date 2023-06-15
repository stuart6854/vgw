#pragma once

#include "../include_old/base.hpp"

#include <vulkan/vulkan.hpp>

namespace VGW_NAMESPACE
{
    class Device;
    struct SwapChain;
    struct Image;

    namespace internal
    {
        auto swap_chain_get_current_image(SwapChain& swapChain) -> Image&;
        auto swap_chain_get_rendering_info(SwapChain& swapChain) -> const vk::RenderingInfo&;

        auto swap_chain_resize(Device& device, SwapChain& swapChain, std::uint32_t width, std::uint32_t height, bool vsync) -> ResultCode;
        auto swap_chain_acquire_next_image(Device& device, SwapChain& swapChain, vk::UniqueSemaphore* outSemaphore) -> ResultCode;
    }
}