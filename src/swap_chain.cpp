#include "vgw/swap_chain.hpp"

#include "vgw/device.hpp"

namespace VGW_NAMESPACE
{
    SwapChain::SwapChain(Device& device, const SwapChainInfo& swapChainInfo) : m_device(&device), m_info(swapChainInfo) {}

    SwapChain::SwapChain(SwapChain&& other) noexcept {}

    auto SwapChain::get_rendering_info() -> vk::RenderingInfo
    {
        ImageViewInfo viewInfo{ .viewType = vk::ImageViewType::e2D, .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
        auto view = get_current_image()->get_view(viewInfo);
        m_colorAttachment.setImageView(view);

        return m_renderingInfo;
    }

    auto SwapChain::resize(std::uint32_t width, std::uint32_t height, bool vsync) -> ResultCode
    {
        m_images.clear();

        vk::SurfaceFormatKHR surfaceFormat = { vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear };

        vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
        swapChainCreateInfo.setSurface(m_info.surface);
        swapChainCreateInfo.setMinImageCount(3);
        swapChainCreateInfo.setImageFormat(surfaceFormat.format);
        swapChainCreateInfo.setImageColorSpace(surfaceFormat.colorSpace);
        swapChainCreateInfo.setImageExtent({ m_info.width, m_info.height });
        swapChainCreateInfo.setImageArrayLayers(1);
        swapChainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
        swapChainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapChainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapChainCreateInfo.setPresentMode(vk::PresentModeKHR::eMailbox);
        swapChainCreateInfo.setClipped(VK_TRUE);
        swapChainCreateInfo.setOldSwapchain(m_swapChain.get());
        m_swapChain = m_device->get_device().createSwapchainKHRUnique(swapChainCreateInfo).value;

        ImageInfo imageInfo{
            .width = m_info.width,
            .height = m_info.height,
            .depth = 1,
            .mipLevels = 1,
            .format = surfaceFormat.format,
            .usage = vk::ImageUsageFlagBits::eColorAttachment,
        };
        auto swapChainImages = m_device->get_device().getSwapchainImagesKHR(m_swapChain.get()).value;
        m_images.resize(swapChainImages.size());
        for (auto i = 0; i < m_images.size(); ++i)
        {
            m_images[i] = std::make_unique<Image>(*this, swapChainImages[i], imageInfo);
        }

        VGW_ASSERT(!m_images.empty());

        m_colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        m_colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        m_colorAttachment.setImageLayout(vk::ImageLayout::eAttachmentOptimal);

        m_renderingInfo.setColorAttachments(m_colorAttachment);
        m_renderingInfo.setRenderArea({ { 0, 0 }, { m_info.width, m_info.height } });
        m_renderingInfo.setLayerCount(1);

        return ResultCode::eSuccess;
    }

    void SwapChain::acquire_next_image(vk::UniqueSemaphore* outSemaphore)
    {
        vk::Semaphore semaphore{};
        if (outSemaphore != nullptr)
        {
            vk::SemaphoreCreateInfo semaphoreInfo{};
            *outSemaphore = m_device->get_device().createSemaphoreUnique(semaphoreInfo).value;
            semaphore = outSemaphore->get();
        }

        m_imageIndex = m_device->get_device().acquireNextImageKHR(m_swapChain.get(), std::uint64_t(-1), semaphore, {}).value;
    }

    void SwapChain::present(std::uint32_t queueIndex)
    {
        is_invariant();

        m_device->present(*this, queueIndex);
    }

    void SwapChain::is_invariant()
    {
        VGW_ASSERT(m_device);
        VGW_ASSERT(m_info.surface);
        VGW_ASSERT(m_swapChain);
    }

}