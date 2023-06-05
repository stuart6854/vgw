#include "vgw/image.hpp"

#include "vgw/device.hpp"
#include "vgw/swap_chain.hpp"

#include <vulkan/vulkan_hash.hpp>

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
        resize(m_extent.width, m_extent.height, m_extent.depth);
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
        destroy();
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
        viewCreateInfo.setFormat(m_format);
        viewCreateInfo.setSubresourceRange(viewInfo.subresourceRange);

        m_viewMap[viewHash] = m_device->get_device().createImageViewUnique(viewCreateInfo).value;
        return m_viewMap.at(viewHash).get();
    }

    void Image::destroy()
    {
        if (!m_isSwapChainImage)
        {
            auto allocator = m_device->get_allocator();
            allocator.destroyImage(m_image, m_allocation);
            m_image = nullptr;
            m_allocation = nullptr;
        }
    }

    void Image::resize(std::uint32_t width, std::uint32_t height, std::uint32_t depth)
    {
        is_invariant();

        destroy();

        m_extent = vk::Extent3D(width, height, depth);

        vk::ImageType type{};
        if (m_extent.depth > 1)
        {
            type = vk::ImageType::e3D;
        }
        else if (m_extent.height > 1)
        {
            type = vk::ImageType::e2D;
        }
        else
        {
            type = vk::ImageType::e1D;
        }

        vk::ImageCreateInfo imageInfo{};
        imageInfo.setImageType(type);
        imageInfo.setFormat(m_format);
        imageInfo.setExtent(m_extent);
        imageInfo.setMipLevels(m_mipLevels);
        imageInfo.setArrayLayers(1);
        imageInfo.setSamples(vk::SampleCountFlagBits::e1);
        imageInfo.setUsage(m_usage);

        vma::AllocationCreateInfo allocInfo{};
        allocInfo.setUsage(vma::MemoryUsage::eAuto);

        std::tie(m_image, m_allocation) = m_device->get_allocator().createImage(imageInfo, allocInfo).value;
    }

    void Image::is_invariant()
    {
        VGW_ASSERT(m_device);
        //        VGW_ASSERT(m_image);
        //        VGW_ASSERT(m_allocation || m_isSwapChainImage);
    }

}