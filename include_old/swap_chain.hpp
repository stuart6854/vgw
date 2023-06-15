#pragma once

#include "base.hpp"
#include "image.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace VGW_NAMESPACE
{
    class Device;

    struct SwapChainInfo
    {
        vk::SurfaceKHR surface;
        std::uint32_t width{};
        std::uint32_t height{};
        bool vsync{};
    };

    struct SwapChain
    {
        SwapChain() = default;
        explicit SwapChain(vk::SurfaceKHR surface) : surface(surface) {}

        vk::SurfaceKHR surface;

        vk::SwapchainKHR swapChain{};
        std::uint32_t width{};
        std::uint32_t height{};
        bool vsync{};

        std::vector<Image> images;
        std::uint32_t imageIndex{};

        vk::RenderingAttachmentInfo colorAttachment;
        vk::RenderingInfo renderingInfo;
    };
}