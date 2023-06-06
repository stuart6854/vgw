#include "vgw/image.hpp"

#include "vgw/device.hpp"
#include "vgw/swap_chain.hpp"

#include <vulkan/vulkan_hash.hpp>

namespace VGW_NAMESPACE
{
    Image::Image(Device& device, vk::Image image, vma::Allocation allocation, const ImageInfo& imageInfo)
        : m_device(&device), m_isSwapChainImage(false), m_image(image), m_allocation(allocation), m_info(imageInfo)
    {
    }

    Image::Image(SwapChain& swapChain, vk::Image image, const ImageInfo& imageInfo)
        : m_device(swapChain.get_device()), m_isSwapChainImage(true), m_image(image), m_info(imageInfo)
    {
    }

    Image::~Image()
    {
        if (!m_isSwapChainImage)
        {
            m_device->get_allocator().destroyImage(m_image, m_allocation);
        }
    }

    auto Image::is_valid()
    {
        return m_device && m_image && (m_allocation || m_isSwapChainImage);
    }

    auto Image::get_view(const ImageViewInfo& viewInfo) -> vk::ImageView
    {
        is_invariant();

        std::size_t viewHash{ 0 };
        hash_combine(viewHash, viewInfo.viewType);
        hash_combine(viewHash, viewInfo.subresourceRange);

        if (m_viewMap.contains(viewHash))
        {
            return m_viewMap.at(viewHash).get();
        }

        vk::ImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.setImage(m_image);
        viewCreateInfo.setViewType(viewInfo.viewType);
        viewCreateInfo.setFormat(m_info.format);
        viewCreateInfo.setSubresourceRange(viewInfo.subresourceRange);

        m_viewMap[viewHash] = m_device->get_device().createImageViewUnique(viewCreateInfo).value;
        return m_viewMap.at(viewHash).get();
    }

    void Image::is_invariant()
    {
        VGW_ASSERT(m_device);
        //        VGW_ASSERT(m_image);
        //        VGW_ASSERT(m_allocation || m_isSwapChainImage);
    }

}