#pragma once

#include "base.hpp"

#include <queue>
#include <memory>
#include <vector>
#include <expected>

namespace VGW_NAMESPACE
{
    constexpr auto RESOURCE_STORAGE_MAX_SLOTS = 1024u;

    /**
     * Stores data (default initialise-able type) in a pre-allocated array.
     * The stored data can be access/referenced using handles.
     * @tparam HandleType
     * @tparam ResourceType
     */
    template <typename HandleType, typename ResourceType>
    class DataStorage
    {
    public:
        DataStorage();
        ~DataStorage() = default;

        [[nodiscard]] auto allocate_handle() noexcept -> std::expected<HandleType, ResultCode>;
        auto free_handle(HandleType handle) -> ResultCode;

        auto set_resource(HandleType handle, const ResourceType& resource) -> ResultCode;
        [[nodiscard]] auto get_resource(HandleType handle) noexcept -> std::expected<std::reference_wrapper<ResourceType>, ResultCode>;

        void clear(std::function<void(ResourceType&)>&& destroyFunc = {}) noexcept;

    private:
        struct Slot
        {
            bool isActive{ false };
            HandleType handle;
            ResourceType resource{};
        };
        std::vector<Slot> m_storage;
        std::size_t m_nextFreeSlotIndex{ 0 };

        std::queue<std::uint16_t> m_freeSlotIndices;
    };

    template <typename HandleType, typename ResourceType>
    DataStorage<HandleType, ResourceType>::DataStorage()
    {
        m_storage.resize(RESOURCE_STORAGE_MAX_SLOTS);
        for (auto i = 0u; i < m_storage.size(); ++i)
        {
            m_storage[i].handle = CREATE_HANDLE(HandleType, i, 0u);
        }
    }

    template <typename HandleType, typename ResourceType>
    auto DataStorage<HandleType, ResourceType>::allocate_handle() noexcept -> std::expected<HandleType, ResultCode>
    {
        std::size_t slotIndex{};
        if (m_freeSlotIndices.empty())
        {
            slotIndex = m_nextFreeSlotIndex++;
            auto& slot = m_storage.at(slotIndex);
            slot.isActive = true;
            return slot.handle;
        }

        slotIndex = m_freeSlotIndices.front();
        m_freeSlotIndices.pop();
        auto& slot = m_storage.at(slotIndex);
        slot.isActive = true;
        return slot.handle;
    }

    template <typename HandleType, typename ResourceType>
    auto DataStorage<HandleType, ResourceType>::free_handle(HandleType handle) -> ResultCode
    {
        const auto index = GET_HANDLE_INDEX(handle);
        const auto gen = GET_HANDLE_GEN(handle);

        auto& slot = m_storage.at(index);
        slot.isActive = false;
        slot.handle = CREATE_HANDLE(HandleType, index, gen + 1u);
        m_freeSlotIndices.push(index);

        return ResultCode::eSuccess;
    }

    template <typename HandleType, typename ResourceType>
    auto DataStorage<HandleType, ResourceType>::set_resource(HandleType handle, const ResourceType& resource) -> ResultCode
    {
        const auto index = GET_HANDLE_INDEX(handle);
        auto& slot = m_storage.at(index);
        if (slot.handle != handle)
        {
            return ResultCode::eInvalidHandle;
        }

        slot.resource = resource;

        return ResultCode::eSuccess;
    }

    template <typename HandleType, typename ResourceType>
    auto DataStorage<HandleType, ResourceType>::get_resource(HandleType handle) noexcept
        -> std::expected<std::reference_wrapper<ResourceType>, ResultCode>
    {
        const auto index = GET_HANDLE_INDEX(handle);
        auto& slot = m_storage.at(index);
        if (slot.handle != handle)
        {
            return std::unexpected(ResultCode::eInvalidHandle);
        }

        return std::ref(slot.resource);
    }

    template <typename HandleType, typename ResourceType>
    void DataStorage<HandleType, ResourceType>::clear(std::function<void(ResourceType&)>&& destroyFunc) noexcept
    {
        for (auto i = 0u; i < m_storage.size(); ++i)
        {
            auto& slot = m_storage.at(i);
            slot.handle = CREATE_HANDLE(HandleType, i, 0u);

            if (destroyFunc && slot.isActive)
            {
                destroyFunc(slot.resource);
            }
            slot.isActive = false;
            slot.resource = ResourceType{};
        }
        m_freeSlotIndices = {};
    }

}  // VGW_NAMESPACE
