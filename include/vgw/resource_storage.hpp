#pragma once

#include "base.hpp"

#include <queue>
#include <memory>
#include <vector>
#include <expected>

namespace VGW_NAMESPACE
{
    template <typename HandleType, typename ResourceType>
    class ResourceStorage
    {
    public:
        ResourceStorage() = default;
        ~ResourceStorage() = default;

        [[nodiscard]] auto allocate_handle() noexcept -> std::expected<HandleType, ResultCode>;
        auto free_handle(HandleType handle) -> ResultCode;

        auto set_resource(HandleType handle, std::unique_ptr<ResourceType>&& resource) -> ResultCode;
        [[nodiscard]] auto get_resource(HandleType handle) noexcept -> std::expected<ResourceType*, ResultCode>;

    private:
        struct Slot
        {
            HandleType handle;
            std::unique_ptr<ResourceType> resource{ nullptr };
        };
        std::vector<Slot> m_storage;
        std::size_t m_nextFreeSlotIndex{ 0 };

        std::queue<std::uint16_t> m_freeSlotIndices;
    };

    template <typename HandleType, typename ResourceType>
    auto ResourceStorage<HandleType, ResourceType>::allocate_handle() noexcept -> std::expected<HandleType, ResultCode>
    {
        if (m_freeSlotIndices.empty())
        {
            const auto slotIndex = m_nextFreeSlotIndex++;
            auto handle = CREATE_HANDLE(HandleType, slotIndex, 0u);
            m_storage[slotIndex].handle = handle;
            return handle;
        }

        const auto slotIndex = m_freeSlotIndices.front();
        m_freeSlotIndices.pop();
        const auto handle = m_storage[slotIndex].handle;
        return handle;
    }

    template <typename HandleType, typename ResourceType>
    auto ResourceStorage<HandleType, ResourceType>::free_handle(HandleType handle) -> ResultCode
    {
        const auto index = GET_HANDLE_INDEX(handle);
        const auto gen = GET_HANDLE_GEN(handle);

        m_storage[index].handle = CREATE_HANDLE(HandleType, index, gen + 1u);
        m_freeSlotIndices.push(index);

        return ResultCode::eSuccess;
    }

    //    template <typename HandleType, typename ResourceType>
    //    bool ResourceStorage<HandleType, ResourceType>::is_handle_valid() const noexcept
    //    {
    //        return false;
    //    }

    template <typename HandleType, typename ResourceType>
    auto ResourceStorage<HandleType, ResourceType>::set_resource(HandleType handle, std::unique_ptr<ResourceType>&& resource) -> ResultCode
    {
        const auto index = GET_HANDLE_INDEX(handle);
        auto& slot = m_storage.at(index);
        if (slot.handle != handle)
        {
            return ResultCode::eInvalidHandle;
        }

        slot.resource = std::move(resource);

        return ResultCode::eSuccess;
    }

    template <typename HandleType, typename ResourceType>
    auto ResourceStorage<HandleType, ResourceType>::get_resource(HandleType handle) noexcept -> std::expected<ResourceType*, ResultCode>
    {
        const auto index = GET_HANDLE_INDEX(handle);
        const auto& slot = m_storage.at(index);
        if (slot.handle != handle)
        {
            return std::unexpected(ResultCode::eInvalidHandle);
        }

        return { slot.resource.get() };
    }

}  // VGW_NAMESPACE
