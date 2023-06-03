#include "vgw/swap_chain.hpp"

#include "vgw/device.hpp"

namespace VGW_NAMESPACE
{
    SwapChain::SwapChain(Device& device, const SwapChainInfo& swapChainInfo)
        : m_device(&device), m_surface(swapChainInfo.surface), m_extent(swapChainInfo.width, swapChainInfo.height), m_vsync(swapChainInfo.vsync)
    {
        resize(m_extent.width, m_extent.height, m_vsync);
    }

    SwapChain::SwapChain(SwapChain&& other) noexcept {}

    SwapChain::~SwapChain() {}

    bool SwapChain::is_valid() const
    {
        return m_device && m_surface && m_swapChain;
    }

    void SwapChain::resize(std::uint32_t width, std::uint32_t height, bool vsync)
    {
        m_images.clear();

        vk::SurfaceFormatKHR surfaceFormat = { vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear };

        vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
        swapChainCreateInfo.setSurface(m_surface);
        swapChainCreateInfo.setMinImageCount(3);
        swapChainCreateInfo.setImageFormat(surfaceFormat.format);
        swapChainCreateInfo.setImageColorSpace(surfaceFormat.colorSpace);
        swapChainCreateInfo.setImageExtent(m_extent);
        swapChainCreateInfo.setImageArrayLayers(1);
        swapChainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
        swapChainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapChainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapChainCreateInfo.setPresentMode(vk::PresentModeKHR::eMailbox);
        swapChainCreateInfo.setClipped(VK_TRUE);
        swapChainCreateInfo.setOldSwapchain(m_swapChain.get());
        m_swapChain = m_device->get_device().createSwapchainKHRUnique(swapChainCreateInfo);

        ImageInfo imageInfo{
            .width = m_extent.width,
            .height = m_extent.height,
            .depth = 1,
            .mipLevels = 1,
            .format = surfaceFormat.format,
            .usage = vk::ImageUsageFlagBits::eColorAttachment,
        };
        auto swapChainImages = m_device->get_device().getSwapchainImagesKHR(m_swapChain.get());
        m_images.resize(swapChainImages.size());
        for (auto i = 0; i < m_images.size(); ++i)
        {
            m_images[i] = std::make_unique<Image>(*this, swapChainImages[i], imageInfo);
        }

        VGW_ASSERT(!m_images.empty());
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