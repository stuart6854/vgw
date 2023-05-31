#pragma once

#include "base.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

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

    class Image
    {
    public:
        Image() = default;
        Image(Device& device, const ImageInfo& imageInfo);
        Image(SwapChain& swapChain, vk::Image image, const ImageInfo& imageInfo);
        ~Image();

        /* Getters */

        auto is_valid();

        auto get_image() const -> vk::Image { return m_image; }
//        auto get_view() const -> vk::ImageView { return m_view; }

        auto get_extent() const -> vk::Extent3D { return m_extent; }
        auto get_mip_levels() const -> std::uint32_t { return m_mipLevels; }
        auto get_format() const -> vk::Format { return m_format; }
        auto get_usage() const -> vk::ImageUsageFlags { return m_usage; }

        /* Methods */

        void resize(std::uint32_t width, std::uint32_t height, std::uint32_t depth);

    private:
        void is_invariant();

    private:
        Device* m_device{ nullptr };
        bool m_isSwapChainImage;

        vk::Image m_image;
        vma::Allocation m_allocation;

        vk::Extent3D m_extent;
        std::uint32_t m_mipLevels;
        vk::Format m_format;
        vk::ImageUsageFlags m_usage;
    };
}