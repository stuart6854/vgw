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
        std::uint32_t width;
        std::uint32_t height;
        bool vsync;
    };

    class SwapChain
    {
    public:
        SwapChain() = default;
        SwapChain(Device& device, const SwapChainInfo& swapChainInfo);
        SwapChain(const SwapChain&) = delete;
        SwapChain(SwapChain&& other) noexcept;
        ~SwapChain() = default;

        /* Getters */

        auto get_device() const -> Device* { return m_device; }

        auto get_info() const noexcept -> const SwapChainInfo& { return m_info; }

        auto get_swap_chain() const -> vk::SwapchainKHR { return m_swapChain.get(); }
        auto get_image_index() const -> std::uint32_t { return m_imageIndex; }

        auto get_current_image() const -> Image* { return m_images[m_imageIndex].get(); }

        auto get_rendering_info() -> vk::RenderingInfo;

        /* Methods */

        auto resize(std::uint32_t width, std::uint32_t height, bool vsync) -> ResultCode;

        void acquire_next_image(vk::UniqueSemaphore* outSemaphore);

        void present(std::uint32_t queueIndex);

    private:
        void is_invariant();

    private:
        Device* m_device;
        SwapChainInfo m_info;

        vk::UniqueSwapchainKHR m_swapChain;
        std::uint32_t m_imageIndex;

        std::vector<std::unique_ptr<Image>> m_images;

        vk::RenderingAttachmentInfo m_colorAttachment;
        vk::RenderingInfo m_renderingInfo;
    };
}