#include "internal_command_buffers.hpp"

#include "internal_device.hpp"

namespace vgw::internal
{
    auto internal_cmd_pool_get(vk::CommandPoolCreateFlagBits poolFlags) -> std::expected<vk::CommandPool, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        const auto it = deviceRef.cmdPoolMap.find(poolFlags);
        if (it != deviceRef.cmdPoolMap.end())
        {
            const auto cmdPool = it->second;
            return cmdPool;
        }

        vk::CommandPoolCreateInfo poolCreateInfo{};
        auto createResult = deviceRef.device.createCommandPool(poolCreateInfo);
        if (createResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::CommandPool ({})!", vk::to_string(poolFlags));
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        deviceRef.cmdPoolMap[poolFlags] = createResult.value;
        return createResult.value;
    }

    auto internal_cmd_buffers_allocate(const CmdBufferAllocInfo& allocInfo) -> std::expected<std::vector<CommandBuffer>, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        auto poolResult = internal_cmd_pool_get(allocInfo.poolFlags);
        if (!poolResult)
        {
            return std::unexpected(poolResult.error());
        }
        const auto pool = poolResult.value();

        vk::CommandBufferAllocateInfo allocCreateInfo{};
        allocCreateInfo.setCommandPool(pool);
        allocCreateInfo.setCommandBufferCount(allocInfo.count);
        allocCreateInfo.setLevel(allocInfo.level);
        auto allocResult = deviceRef.device.allocateCommandBuffers(allocCreateInfo);
        if (allocResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to allocate {} vk::CommandBuffers ({}) from pool({])!",
                      allocInfo.count,
                      vk::to_string(allocInfo.level),
                      vk::to_string(allocInfo.poolFlags));
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        const auto cmdBuffers = allocResult.value;

        std::vector<CommandBuffer> outCmdBuffers{};
        for (auto cmd : cmdBuffers)
        {
            deviceRef.cmdBufferMap[cmd] = CmdBufferData{ pool, std::make_unique<CommandBuffer_T>(cmd) };
            outCmdBuffers.push_back(deviceRef.cmdBufferMap.at(cmd).cmd.get());
        }
        return outCmdBuffers;
    }

    void internal_cmd_buffers_free(const std::vector<CommandBuffer>& cmdBuffers)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        for (const auto& cmd : cmdBuffers)
        {
            auto vkCmd = static_cast<vk::CommandBuffer>(*cmd);
            auto pool = deviceRef.cmdBufferMap.at(vkCmd).pool;

            deviceRef.device.free(pool, vkCmd);
            deviceRef.cmdBufferMap.erase(vkCmd);
        }
    }

    void internal_submit(const SubmitInfo& submitInfo)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        auto queue = deviceRef.queues.at(submitInfo.queueIndex);

        vk::SubmitInfo vkSubmitInfo{};
        vkSubmitInfo.setCommandBuffers(submitInfo.cmdBuffers);
        queue.submit(vkSubmitInfo, submitInfo.signalFence);
    }
}