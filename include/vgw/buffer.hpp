#pragma once

#include "base.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

#include <optional>

namespace VGW_NAMESPACE
{
    class Device;

    struct BufferInfo
    {
        std::uint64_t size;
        vk::BufferUsageFlags usage;
        vma::MemoryUsage memoryUsage;
        vma::AllocationCreateFlags allocationCreateFlags;
    };

    class Buffer
    {
    public:
        Buffer() = default;
        explicit Buffer(Device& device, vk::Buffer buffer, vma::Allocation allocation, const BufferInfo& bufferInfo);
        Buffer(const Buffer&) = delete;
        Buffer(Buffer&& other) noexcept;
        ~Buffer();

        /* Getters  */

        /**
         * Is this a valid/initialised instance?
         */
        bool is_valid() const;

        auto get_buffer() const -> vk::Buffer { return m_buffer; }
        auto get_allocation() const -> vma::Allocation { return m_allocation; }

        auto get_info() const -> const BufferInfo& { return m_info; }
        auto get_size() const -> std::uint64_t { return m_info.size; }

        /* Methods */

        void resize(std::size_t newSize);

        auto map() -> void*;
        void unmap();

        /* Operators */

        auto operator=(const Buffer&) -> Buffer& = delete;
        auto operator=(Buffer&& rhs) noexcept -> Buffer&;

    private:
        /* Checks if this instance is invariant. Used to check pre-/post-conditions for methods. */
        void is_invariant();

    private:
        Device* m_device{ nullptr };
        vk::Buffer m_buffer;
        vma::Allocation m_allocation;

        BufferInfo m_info;
    };
}