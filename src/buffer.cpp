#include "vgw/buffer.hpp"

#include "vgw/device.hpp"

namespace VGW_NAMESPACE
{
    Buffer::Buffer(Device& device,
                   std::uint64_t size,
                   vk::BufferUsageFlags usage,
                   vma::MemoryUsage memoryUsage,
                   vma::AllocationCreateFlags allocationCreateFlags)
        : m_device(&device), m_size(size), m_usage(usage), m_memoryUsage(memoryUsage), m_allocationFlags(allocationCreateFlags)
    {
        resize(m_size);
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
        auto allocator = m_device->get_allocator();
        allocator.destroyBuffer(m_buffer, m_allocation);
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

    void Buffer::resize(std::size_t newSize)
    {
        auto allocator = m_device->get_allocator();
        if (m_buffer)
        {
            allocator.destroyBuffer(m_buffer, m_allocation);
        }

        m_size = newSize;

        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.setSize(m_size);
        bufferInfo.setUsage(m_usage);

        vma::AllocationCreateInfo allocInfo{};
        allocInfo.setUsage(m_memoryUsage);
        allocInfo.setFlags(m_allocationFlags);

        std::tie(m_buffer, m_allocation) = m_device->get_allocator().createBuffer(bufferInfo, allocInfo);

        is_invariant();
    }

    auto Buffer::map() -> void*
    {
        is_invariant();

        auto allocator = m_device->get_allocator();
        void* mapped = allocator.mapMemory(m_allocation).value;
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