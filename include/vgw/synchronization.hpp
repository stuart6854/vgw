#pragma once

#include "base.hpp"

#include <vulkan/vulkan.hpp>

namespace VGW_NAMESPACE
{
    class Device;

    class Fence
    {
    public:
        Fence() = default;
        Fence(Device& device, vk::Fence fence);
        Fence(const Fence&) = delete;
        Fence(Fence&& other) noexcept;
        ~Fence();

        /* Getters */

        bool is_valid() const;

        auto get_fence() const -> vk::Fence { return m_fence; }

        /* Methods */

        void destroy();

        void wait();

        void reset();

        /* Operators */

        auto operator=(const Fence&) -> Fence& = delete;
        auto operator=(Fence&& rhs) noexcept -> Fence&;

    private:
        /* Checks if this instance is invariant. Used to check pre-/post-conditions for methods. */
        void is_invariant();

    private:
        Device* m_device{ nullptr };
        vk::Fence m_fence;
    };
}
