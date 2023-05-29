#include "vgw/buffer.hpp"

#include "vgw/device.hpp"
#include "vulkan-memory-allocator-hpp/vk_mem_alloc_handles.hpp"

namespace VGW_NAMESPACE
{
    Buffer::Buffer(Device& device,
                   vk::Buffer buffer,
                   vma::Allocation allocation,
                   const vk::BufferCreateInfo& bufferInfo,
                   const vma::AllocationCreateInfo& allocInfo)
        : m_device(&device),
          m_buffer(buffer),
          m_allocation(allocation),
          m_size(bufferInfo.size),
          m_usage(bufferInfo.usage),
          m_memoryUsage(allocInfo.usage),
          m_allocationFlags(allocInfo.flags)
    {
    }

    Buffer::Buffer(Buffer&& other) noexcept
    {
        std::swap(m_device, other.m_device);
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_allocation, other.m_allocation);
        std::swap(m_size, other.m_size);
        std::swap(m_usage, other.m_usage);
        std::swap(m_memoryUsage, other.m_memoryUsage);
        std::swap(m_allocationFlags, other.m_allocationFlags);
    }

    Buffer::~Buffer()
    {
        destroy();
    }

    bool Buffer::is_valid() const
    {
        return m_device && m_buffer && m_allocation;
    }

    void Buffer::set_usage(vk::BufferUsageFlags usage)
    {
        m_usage = usage;
    }

    void Buffer::set_memory_usage(vma::MemoryUsage memoryUsage)
    {
        m_memoryUsage = memoryUsage;
    }

    void Buffer::set_allocation_flags(vma::AllocationCreateFlags allocationFlags)
    {
        m_allocationFlags = allocationFlags;
    }

    void Buffer::destroy()
    {
        if (is_valid())
        {
            auto allocator = m_device->get_allocator();
            allocator.destroyBuffer(m_buffer, m_allocation);
            m_buffer = nullptr;
            m_allocation = nullptr;
        }
    }

    void Buffer::resize(std::size_t newSize)
    {
        is_invariant();

        destroy();

        m_size = newSize;
        *this = m_device->create_buffer(newSize, m_usage, m_memoryUsage, m_allocationFlags);

        is_invariant();
    }

    auto Buffer::map() -> void*
    {
        is_invariant();

        auto allocator = m_device->get_allocator();
        void* mapped = allocator.mapMemory(m_allocation);
        return mapped;
    }

    void Buffer::unmap()
    {
        is_invariant();

        auto allocator = m_device->get_allocator();
        allocator.unmapMemory(m_allocation);
    }

    auto Buffer::operator=(Buffer&& rhs) noexcept -> Buffer&
    {
        std::swap(m_device, rhs.m_device);
        std::swap(m_buffer, rhs.m_buffer);
        std::swap(m_allocation, rhs.m_allocation);
        std::swap(m_size, rhs.m_size);
        std::swap(m_usage, rhs.m_usage);
        std::swap(m_memoryUsage, rhs.m_memoryUsage);
        std::swap(m_allocationFlags, rhs.m_allocationFlags);
        return *this;
    }

    void Buffer::is_invariant()
    {
        VGW_ASSERT(m_device);
        VGW_ASSERT(m_buffer);
        VGW_ASSERT(m_allocation);
    }

}