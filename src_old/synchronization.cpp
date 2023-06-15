#include "../include_old/synchronization.hpp"

#include "../include_old/device.hpp"

#include <limits>

namespace VGW_NAMESPACE
{
    Fence::Fence(Device& device, vk::Fence fence) : m_device(&device), m_fence(fence) {}

    Fence::Fence(Fence&& other) noexcept : m_device()
    {
        std::swap(m_device, other.m_device);
        std::swap(m_fence, other.m_fence);
    }

    Fence::~Fence()
    {
        destroy();
    }

    bool Fence::is_valid() const
    {
        return m_device && m_fence;
    }

    void Fence::destroy()
    {
        if (is_valid())
        {
            m_device->get_device().destroy(m_fence);
            m_fence = nullptr;
        }
    }

    void Fence::wait()
    {
        is_invariant();

        auto device = m_device->get_device();
        const auto timeout = std::numeric_limits<std::uint64_t>::max();
        auto result = device.waitForFences(m_fence, VK_TRUE, timeout);
        VGW_UNUSED(result);
    }

    void Fence::reset()
    {
        m_device->get_device().resetFences(m_fence);
    }

    auto Fence::operator=(Fence&& rhs) noexcept -> Fence&
    {
        std::swap(m_device, rhs.m_device);
        std::swap(m_fence, rhs.m_fence);
        return *this;
    }

    void Fence::is_invariant()
    {
        VGW_ASSERT(m_device);
        VGW_ASSERT(m_fence);
    }
}