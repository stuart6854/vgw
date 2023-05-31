#include "vgw/image.hpp"

#include "vgw/device.hpp"
#include "vgw/swap_chain.hpp"

namespace VGW_NAMESPACE
{
    Image::Image(Device& device, const ImageInfo& imageInfo)
        : m_device(&device),
          m_isSwapChainImage(false),
          m_extent(imageInfo.width, imageInfo.height, imageInfo.depth),
          m_mipLevels(imageInfo.mipLevels),
          m_format(imageInfo.format),
          m_usage(imageInfo.usage)
    {
        resize(m_extent.width, m_extent.height, m_extent.height);
    }

    Image::Image(SwapChain& swapChain, vk::Image image, const ImageInfo& imageInfo)
        : m_device(swapChain.get_device()),
          m_isSwapChainImage(true),
          m_image(image),
          m_extent(imageInfo.width, imageInfo.height, 1),
          m_mipLevels(1),
          m_format(imageInfo.format),
          m_usage(imageInfo.usage)
    {
    }

    Image::~Image()
    {
        if (!m_isSwapChainImage)
        {
            auto allocator = m_device->get_allocator();
            allocator.destroyImage(m_image, m_allocation);
            m_image = nullptr;
            m_allocation = nullptr;
        }
    }

    auto Image::is_valid()
    {
        return m_device && m_image && (m_allocation || m_isSwapChainImage);
    }

    void Image::is_invariant()
    {
        VGW_ASSERT(m_device);
        VGW_ASSERT(m_image);
        VGW_ASSERT(m_allocation || m_isSwapChainImage);
    }

    void Image::resize(std::uint32_t width, std::uint32_t height, std::uint32_t depth) {}
}