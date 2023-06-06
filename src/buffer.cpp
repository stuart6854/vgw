#include "vgw/buffer.hpp"

#include "vgw/device.hpp"

namespace VGW_NAMESPACE
{
    Buffer::Buffer(Device& device, vk::Buffer buffer, vma::Allocation allocation, const BufferInfo& bufferInfo)
        : m_device(&device), m_buffer(buffer), m_allocation(allocation), m_info(bufferInfo)
    {
    }

    Buffer::Buffer(Buffer&& other) noexcept
    {
        std::swap(m_device, other.m_device);
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_allocation, other.m_allocation);
        std::swap(m_info, other.m_info);
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
        std::swap(m_info, rhs.m_info);
        return *this;
    }

    void Buffer::is_invariant()
    {
        VGW_ASSERT(m_device);
        VGW_ASSERT(m_buffer);
        VGW_ASSERT(m_allocation);
    }

}