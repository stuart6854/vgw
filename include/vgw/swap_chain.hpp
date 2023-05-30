#pragma once

#include "base.hpp"

#include <vulkan/vulkan.hpp>

namespace VGW_NAMESPACE
{
    class Device;

    class SwapChain
    {
    public:
        SwapChain() = default;
        SwapChain(Device& device, vk::SurfaceKHR surface, std::uint32_t width, std::uint32_t height, bool vsync);
        SwapChain(const SwapChain&) = delete;
        SwapChain(SwapChain&& other) noexcept;
        ~SwapChain();

        /* Getters */

        bool is_valid() const;

        auto get_swap_chain() const -> vk::SwapchainKHR { return m_swapChain.get(); }
        auto get_image_index() const -> std::uint32_t { return m_imageIndex; }

        /* Methods */

        void resize(std::uint32_t width, std::uint32_t height, bool vsync);

        void acquire_next_image(vk::UniqueSemaphore* outSemaphore);

        void present(std::uint32_t queueIndex);

    private:
        void is_invariant();

    private:
        Device* m_device;
        vk::SurfaceKHR m_surface;

        vk::Extent2D m_extent;
        bool m_vsync;

        vk::UniqueSwapchainKHR m_swapChain;
        std::uint32_t m_imageIndex;
    };
}