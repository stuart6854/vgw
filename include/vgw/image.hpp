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

    struct ImageViewInfo
    {
        vk::ImageViewType viewType;
        vk::ImageSubresourceRange subresourceRange;
    };

    class Image
    {
    public:
        Image() = default;
        Image(Device& device, vk::Image image, vma::Allocation allocation, const ImageInfo& imageInfo);
        Image(SwapChain& swapChain, vk::Image image, const ImageInfo& imageInfo);
        ~Image();

        /* Getters */

        auto is_valid();

        auto get_image() const -> vk::Image { return m_image; }

        auto get_info() const -> const ImageInfo& { return m_info; }

        auto get_view(const ImageViewInfo& viewInfo) -> vk::ImageView;

        /* Methods */

    private:
        void is_invariant();

    private:
        Device* m_device{ nullptr };
        bool m_isSwapChainImage;

        vk::Image m_image;
        vma::Allocation m_allocation;

        ImageInfo m_info;

        std::unordered_map<std::size_t, vk::UniqueImageView> m_viewMap;
    };
}