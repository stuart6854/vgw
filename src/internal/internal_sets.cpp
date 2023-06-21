#include "internal_sets.hpp"

#include "internal_device.hpp"

namespace vgw::internal
{
    auto internal_sets_allocate(const SetAllocInfo& allocInfo) -> std::expected<std::vector<vk::DescriptorSet>, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        std::vector setLayouts(allocInfo.count, allocInfo.layout);
        vk::DescriptorSetAllocateInfo setAllocInfo{};
        setAllocInfo.setDescriptorPool(deviceRef.descriptorPool);
        setAllocInfo.setSetLayouts(setLayouts);
        auto allocResult = deviceRef.device.allocateDescriptorSets(setAllocInfo);
        if (allocResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to allocate {} descriptor sets!", allocInfo.count);
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        const auto sets = allocResult.value;
        return sets;
    }

    void internal_sets_free(const std::vector<vk::DescriptorSet>& sets)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device.");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        deviceRef.device.free(deviceRef.descriptorPool, sets);
    }

    void internal_sets_bind_buffer(const SetBufferBindInfo& bindInfo)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device.");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        if (deviceRef.setWrites.size() == deviceRef.setWrites.capacity())
        {
            internal_sets_flush_writes();
        }

        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.setBuffer(bindInfo.buffer);
        bufferInfo.setOffset(bindInfo.offset);
        bufferInfo.setRange(bindInfo.range);
        auto& objectRef = deviceRef.setWriteObjects.emplace_back(bufferInfo);
        auto& bufferInfoRef = std::get<vk::DescriptorBufferInfo>(objectRef);

        auto& writeRef = deviceRef.setWrites.emplace_back();
        writeRef.setDstSet(bindInfo.set);
        writeRef.setDstBinding(bindInfo.binding);
        writeRef.setDstArrayElement(0);
        writeRef.setDescriptorCount(1);
        writeRef.setDescriptorType(bindInfo.type);
        writeRef.setPBufferInfo(&bufferInfoRef);
    }

    void internal_sets_bind_image(const SetImageBindInfo& bindInfo)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device.");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        if (deviceRef.setWrites.size() == deviceRef.setWrites.capacity())
        {
            internal_sets_flush_writes();
        }

        vk::DescriptorImageInfo imageInfo{};
        imageInfo.setSampler(bindInfo.sampler);
        imageInfo.setImageView(bindInfo.imageView);
        imageInfo.setImageLayout(bindInfo.imageLayout);
        auto& objectRef = deviceRef.setWriteObjects.emplace_back(imageInfo);
        auto& imageInfoRef = std::get<vk::DescriptorImageInfo>(objectRef);

        auto& writeRef = deviceRef.setWrites.emplace_back();
        writeRef.setDstSet(bindInfo.set);
        writeRef.setDstBinding(bindInfo.binding);
        writeRef.setDstArrayElement(0);
        writeRef.setDescriptorCount(1);
        writeRef.setDescriptorType(bindInfo.type);
        writeRef.setPImageInfo(&imageInfoRef);
    }

    void internal_sets_flush_writes()
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device.");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        if (deviceRef.setWrites.empty())
        {
            return;
        }

        deviceRef.device.updateDescriptorSets(deviceRef.setWrites, {});
        deviceRef.setWrites.clear();
        deviceRef.setWriteObjects.clear();
    }

    void internal_sets_bind(vk::CommandBuffer cmdBuffer,
                            vk::Pipeline pipeline,
                            std::uint32_t firstSet,
                            const std::vector<vk::DescriptorSet>& sets)
    {
        auto getResult = internal_pipeline_get(pipeline);
        if (!getResult)
        {
            log_error("Failed to get pipeline!");
            return;
        }
        auto& pipelineRef = getResult.value().get();

        cmdBuffer.bindDescriptorSets(pipelineRef.bindPoint, pipelineRef.layout, firstSet, sets, {});
    }

}