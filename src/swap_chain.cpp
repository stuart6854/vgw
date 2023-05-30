#include "vgw/swap_chain.hpp"

#include "vgw/device.hpp"

namespace VGW_NAMESPACE
{
    SwapChain::SwapChain(Device& device, vk::SurfaceKHR surface, std::uint32_t width, std::uint32_t height, bool vsync)
        : m_device(&device), m_surface(surface), m_extent(width, height), m_vsync(vsync)
    {
        resize(width, height, vsync);
    }

    SwapChain::SwapChain(SwapChain&& other) noexcept {}

    SwapChain::~SwapChain() {}

    bool SwapChain::is_valid() const
    {
        return m_device && m_surface && m_swapChain;
    }

    void SwapChain::resize(std::uint32_t width, std::uint32_t height, bool vsync)
    {
        vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
        swapChainCreateInfo.setSurface(m_surface);
        swapChainCreateInfo.setMinImageCount(3);
        swapChainCreateInfo.setImageFormat(vk::Format::eB8G8R8A8Srgb);
        swapChainCreateInfo.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
        swapChainCreateInfo.setImageExtent(m_extent);
        swapChainCreateInfo.setImageArrayLayers(1);
        swapChainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapChainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapChainCreateInfo.setPresentMode(vk::PresentModeKHR::eMailbox);
        swapChainCreateInfo.setClipped(VK_TRUE);
        swapChainCreateInfo.setOldSwapchain(m_swapChain.get());
        m_swapChain = m_device->get_device().createSwapchainKHRUnique(swapChainCreateInfo);
    }

    void SwapChain::acquire_next_image(vk::UniqueSemaphore* outSemaphore)
    {
        vk::Semaphore semaphore{};
        if (outSemaphore != nullptr)
        {
            vk::SemaphoreCreateInfo semaphoreInfo{};
            *outSemaphore = m_device->get_device().createSemaphoreUnique(semaphoreInfo);
            semaphore = outSemaphore->get();
        }

        m_imageIndex = m_device->get_device().acquireNextImageKHR(m_swapChain.get(), std::uint64_t(-1), semaphore, {}).value;
    }

    void SwapChain::present(std::uint32_t queueIndex)
    {
        is_invariant();

        m_device->present(queueIndex, *this);
    }

    void SwapChain::is_invariant()
    {
        VGW_ASSERT(m_device);
        VGW_ASSERT(m_surface);
        VGW_ASSERT(m_swapChain);
    }
}