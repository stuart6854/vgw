#pragma once

#include "base.hpp"

#include <vulkan/vulkan.hpp>
#include "vulkan-memory-allocator-hpp/vk_mem_alloc.hpp"

namespace VGW_NAMESPACE
{
    class Device;
    class SwapChain;

    struct ImageInfo
    {
        std::uint32_t width;
        std::uint32_t height{ 1 };
        std::uint32_t depth{ 1 };
        std::uint32_t mipLevels{ 1 };
        vk::Format format;
        vk::ImageUsageFlags usage;
    };

    struct ImageViewInfo
    {
        vk::ImageViewType viewType;
        vk::ImageSubresourceRange subresourceRange;
    };

    struct Image
    {
        Image() = default;
        Image(vk::Image image, vma::Allocation allocation, const ImageInfo& imageInfo)
            : m_isSwapChainImage(false),
              m_image(image),
              m_allocation(allocation),
              width(imageInfo.width),
              height(imageInfo.height),
              depth(imageInfo.depth),
              mipLevels(imageInfo.mipLevels),
              format(imageInfo.format),
              usage(imageInfo.usage)
        {
        }

        Image(SwapChain& swapChain, vk::Image image, const ImageInfo& imageInfo)
            : m_isSwapChainImage(true),
              m_image(image),
              width(imageInfo.width),
              height(imageInfo.height),
              depth(imageInfo.depth),
              mipLevels(imageInfo.mipLevels),
              format(imageInfo.format),
              usage(imageInfo.usage)
        {
        }

        bool m_isSwapChainImage{};

        vk::Image m_image;
        vma::Allocation m_allocation;

        std::uint32_t width{};
        std::uint32_t height{ 1 };
        std::uint32_t depth{ 1 };
        std::uint32_t mipLevels{ 1 };
        vk::Format format{};
        vk::ImageUsageFlags usage{};

        std::unordered_map<std::size_t, vk::ImageView> m_viewMap;
    };
}