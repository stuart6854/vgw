#include "vgw/device.hpp"

#include "device_helpers.hpp"
#include "vgw/image.hpp"
#include "vgw/buffer.hpp"
#include "vgw/context.hpp"
#include "vgw/pipelines.hpp"
#include "vgw/swap_chain.hpp"
#include "vgw/render_pass.hpp"
#include "vgw/command_buffer.hpp"
#include "vgw/synchronization.hpp"

#include <vulkan/vulkan_hash.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <unordered_map>

namespace VGW_NAMESPACE
{
    Device::Device(Context& context, const DeviceInfo& deviceInfo) : m_context(&context)
    {
        if (deviceInfo.wantedQueues.empty())
        {
            m_context->log_error("Device must be create with at least 1 queue!");
            return;
        }

#pragma region Physical Device Selection

        auto instance = m_context->get_instance();
        std::uint32_t x{ 0 };
        vkEnumeratePhysicalDevices(instance, &x, nullptr);
        const auto& availablePhysicalDevices = instance.enumeratePhysicalDevices().value;
        if (availablePhysicalDevices.empty())
        {
            return;
        }

        m_physicalDevice = availablePhysicalDevices.front();  // #TODO: Implement physical device selection/scoring.

#pragma endregion

#pragma region Layers & Extensions

        std::vector<const char*> wantedExtensions;
        if (deviceInfo.enableSwapChains)
        {
            wantedExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }
        if (deviceInfo.enableDynamicRendering)
        {
            wantedExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        }

        std::vector<const char*> enabledExtensions;
        for (const auto& extension : wantedExtensions)
        {
            if (is_device_extension_supported(m_physicalDevice, extension))
            {
                enabledExtensions.push_back(extension);
            }
            else
            {
                auto msg = std::format("Extension <{}> is not supported. It will not be enabled.", extension);
                m_context->log_error(msg);
            }
        }

        m_context->log_info("Enabled extensions:");
        for (const auto& extension : enabledExtensions)
        {
            auto msg = std::format("  {}", extension);
            m_context->log_info(msg);
        }

#pragma endregion

#pragma region Queues

        auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();

        std::unordered_map<std::uint32_t, std::uint32_t> queueFamilyCounts;
        std::vector<std::optional<std::uint32_t>> wantedQueueFamilies;
        for (auto i = 0; i < deviceInfo.wantedQueues.size(); ++i)
        {
            auto wantedQueue = deviceInfo.wantedQueues.at(i);
            bool foundFamily = false;
            for (auto familyIndex = 0; familyIndex < queueFamilies.size(); ++familyIndex)
            {
                const auto& family = queueFamilies.at(familyIndex);

                if (family.queueFlags & wantedQueue)
                {
                    if (queueFamilyCounts.contains(familyIndex) && queueFamilyCounts.at(familyIndex) >= family.queueCount)
                    {
                        continue;
                    }

                    queueFamilyCounts[familyIndex]++;
                    wantedQueueFamilies.emplace_back(familyIndex);
                    foundFamily = true;
                    break;
                }
            }
            if (!foundFamily)
            {
                auto msg = std::format("Unable to create queue at index {}!", i);
                m_context->log_error(msg);
            }
        }

        const std::vector<float> QueuePriorities(32, 1.0f);
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfoVec;
        for (auto& [family, count] : queueFamilyCounts)
        {
            auto& queueCreateInfo = queueCreateInfoVec.emplace_back();
            queueCreateInfo.setQueueFamilyIndex(family);
            queueCreateInfo.setQueueCount(count);
            queueCreateInfo.setPQueuePriorities(QueuePriorities.data());
        }

#pragma endregion

        m_dynamicRenderingSupported =
            std::ranges::find(enabledExtensions, std::string_view(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)) != enabledExtensions.end();

        void* nextFeature{ nullptr };
        vk::PhysicalDeviceSynchronization2Features synchronization2Features{ true };
        nextFeature = &synchronization2Features;

        vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{ true };
        if (m_dynamicRenderingSupported)
        {
            if (nextFeature)
            {
                dynamicRenderingFeatures.setPNext(nextFeature);
            }
            nextFeature = &dynamicRenderingFeatures;
        }

        vk::PhysicalDeviceFeatures enabledFeatures;
        enabledFeatures.setFillModeNonSolid(true);
        enabledFeatures.setWideLines(true);

        vk::DeviceCreateInfo deviceCreateInfo{ vk::DeviceCreateFlags(), queueCreateInfoVec, {},
                                               enabledExtensions,       &enabledFeatures,   nextFeature };
        m_device = m_physicalDevice.createDeviceUnique(deviceCreateInfo).value;
        if (!m_device)
        {
            m_context->log_error("Failed to create vk::Device!", std::source_location::current());
            return;
        }

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device.get());

        queueFamilyCounts.clear();
        for (auto queueFamily : wantedQueueFamilies)
        {
            const auto index = queueFamilyCounts[queueFamily.value()];
            m_queues.emplace_back(m_device->getQueue(queueFamily.value(), index));
            queueFamilyCounts[queueFamily.value()]++;

            VGW_ASSERT(m_queues.back());
        }

        vma::AllocatorCreateInfo allocatorInfo{};
        allocatorInfo.setInstance(m_context->get_instance());
        allocatorInfo.setPhysicalDevice(m_physicalDevice);
        allocatorInfo.setDevice(m_device.get());
        allocatorInfo.setVulkanApiVersion(VK_API_VERSION_1_3);
        m_allocator = vma::createAllocatorUnique(allocatorInfo).value;
        if (!m_allocator)
        {
            m_context->log_error("Failed to create vma::Allocator!", std::source_location::current());
            return;
        }

        vk::DescriptorPoolCreateInfo poolCreateInfo{};
        poolCreateInfo.setMaxSets(deviceInfo.maxDescriptorSets);
        poolCreateInfo.setPoolSizes(deviceInfo.descriptorPoolSizes);
        poolCreateInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
        m_descriptorPool = m_device->createDescriptorPoolUnique(poolCreateInfo).value;

        is_invariant();
    }

    Device::Device(Device&& other) noexcept
    {
        std::swap(m_context, other.m_context);
        std::swap(m_physicalDevice, other.m_physicalDevice);
        std::swap(m_device, other.m_device);
        std::swap(m_allocator, other.m_allocator);
        std::swap(m_queues, other.m_queues);
        std::swap(m_descriptorPool, other.m_descriptorPool);
        std::swap(m_commandPoolMap, other.m_commandPoolMap);
        std::swap(m_descriptorSetLayoutMap, other.m_descriptorSetLayoutMap);
    }

    Device::~Device()
    {
        destroy();
    }

    bool Device::is_valid() const
    {
        return m_context && m_device && m_allocator;
    }

    auto Device::get_context() -> Context&
    {
        is_invariant();
        return *m_context;
    }

    void Device::destroy()
    {
        if (is_valid())
        {
            m_pendingDescriptorWrites.clear();
            m_pendingBufferInfos.clear();

            m_descriptorSetLayoutMap.clear();
            m_commandPoolMap.clear();

            m_descriptorPool.reset();
            m_queues.clear();
            m_allocator.reset();

            m_allocator.reset();
            m_device.reset();
        }
    }

    auto Device::create_swap_chain(const SwapChainInfo& swapChainInfo) -> std::unique_ptr<SwapChain>
    {
        return std::make_unique<SwapChain>(*this, swapChainInfo);
    }

    auto Device::create_command_buffers(std::uint32_t count, vk::CommandPoolCreateFlags poolFlags)
        -> std::vector<std::unique_ptr<CommandBuffer>>
    {
        is_invariant();
        VGW_ASSERT(count >= 1);

        poolFlags |= vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        const auto poolFlagsHash = std::hash<vk::CommandPoolCreateFlags>{}(poolFlags);
        if (!m_commandPoolMap.contains(poolFlagsHash))
        {
            vk::CommandPoolCreateInfo commandPoolInfo{};
            commandPoolInfo.setFlags(poolFlags);
            m_commandPoolMap[poolFlagsHash] = m_device->createCommandPoolUnique(commandPoolInfo).value;
        }

        const auto commandPool = m_commandPoolMap.at(poolFlagsHash).get();

        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        allocInfo.setCommandPool(commandPool);
        allocInfo.setCommandBufferCount(count);
        auto allocatedCommandBuffers = m_device->allocateCommandBuffers(allocInfo).value;

        std::vector<std::unique_ptr<CommandBuffer>> commandBuffers;
        commandBuffers.reserve(count);
        for (auto i = 0; i < count; ++i)
        {
            commandBuffers.emplace_back(std::make_unique<CommandBuffer>(*this, commandPool, allocatedCommandBuffers[i]));
        }

        return commandBuffers;
    }

    auto Device::create_buffer(const BufferInfo& bufferInfo) noexcept -> std::expected<HandleBuffer, ResultCode>
    {
        is_invariant();

        auto handleResult = m_buffers.allocate_handle();
        if (!handleResult)
        {
            return std::unexpected(handleResult.error());
        }
        const auto handle = handleResult.value();

        auto createResult = allocate_buffer(bufferInfo);
        if (!createResult)
        {
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        auto [vkBuffer, vmaAllocation] = createResult.value();
        auto buffer = std::make_unique<Buffer>(*this, vkBuffer, vmaAllocation, bufferInfo);

        auto result = m_buffers.set_resource(handle, std::move(buffer));
        if (result != ResultCode::eSuccess)
        {
            return std::unexpected(result);
        }

        return { handle };
    }

    auto Device::resize_buffer(HandleBuffer handle) noexcept -> ResultCode
    {
        is_invariant();
#if 0
        if (!m_buffers.is_handle_valid())
        {
            return ResultCode::eInvalidHandle;
        }

        auto result = m_buffers.get_resource(handle);
        if (!result)
        {
            return result.error();
        }

        const auto& oldBuffer = result.value().get();
        const auto& bufferInfo = oldBuffer.get_info();

        auto createResult = allocate_buffer(bufferInfo);
        if (!createResult)
        {
            return ResultCode::eFailedToCreate;
        }

        auto [vkBuffer, vmaAllocation] = createResult.value();
        auto buffer = std::make_unique<Buffer>(*this, vkBuffer, vmaAllocation, bufferInfo);

        auto swapResult = m_buffers.set_handle_resource(handle, std::move(buffer));
        if (!swapResult)
        {
            return swapResult.error();
        }

        auto oldBufferPtr = std::move(swapResult.value());
        // #TODO: Destroy buffer
#endif

        return ResultCode::eSuccess;
    }

    auto Device::get_buffer(HandleBuffer handle) noexcept -> std::expected<std::reference_wrapper<Buffer>, ResultCode>
    {
        auto result = m_buffers.get_resource(handle);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        auto* ptr = result.value();
        VGW_ASSERT(ptr != nullptr);
        return { *ptr };
    }

    void Device::destroy_buffer(HandleBuffer handle) noexcept
    {
        // #TODO: Handle safe deletion? Or leave to library consumer?
        auto result = m_buffers.set_resource(handle, nullptr);
        if (result != ResultCode::eSuccess)
        {
            return;
        }

        m_buffers.free_handle(handle);
    }

    auto Device::create_staging_buffer(std::uint64_t size) -> std::expected<HandleBuffer, ResultCode>
    {
        return create_buffer({ size,
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vma::MemoryUsage::eAuto,
                               vma::AllocationCreateFlagBits::eHostAccessSequentialWrite });
    }

    auto Device::create_storage_buffer(std::uint64_t size) -> std::expected<HandleBuffer, ResultCode>
    {
        return create_buffer(
            { size, vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eAutoPreferDevice, vma::AllocationCreateFlags() });
    }

    auto Device::create_uniform_buffer(std::uint64_t size) -> std::expected<HandleBuffer, ResultCode>
    {
        return create_buffer({ size,
                               vk::BufferUsageFlagBits::eUniformBuffer,
                               vma::MemoryUsage::eAutoPreferDevice,
                               vma::AllocationCreateFlagBits::eHostAccessSequentialWrite });
    }

    auto Device::create_vertex_buffer(std::uint64_t size) -> std::expected<HandleBuffer, ResultCode>
    {
        return create_buffer(
            { size, vk::BufferUsageFlagBits::eVertexBuffer, vma::MemoryUsage::eAutoPreferDevice, vma::AllocationCreateFlags() });
    }

    auto Device::create_index_buffer(std::uint64_t size) -> std::expected<HandleBuffer, ResultCode>
    {
        return create_buffer(
            { size, vk::BufferUsageFlagBits::eIndexBuffer, vma::MemoryUsage::eAutoPreferDevice, vma::AllocationCreateFlags() });
    }

    auto Device::create_image(const ImageInfo& imageInfo) noexcept -> std::expected<HandleImage, ResultCode>
    {
        is_invariant();

        auto handleResult = m_images.allocate_handle();
        if (!handleResult)
        {
            return std::unexpected(handleResult.error());
        }
        const auto handle = handleResult.value();

        auto createResult = allocate_image(imageInfo);
        if (!createResult)
        {
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        auto [vkImage, vmaAllocation] = createResult.value();
        auto buffer = std::make_unique<Image>(*this, vkImage, vmaAllocation, imageInfo);

        auto result = m_images.set_resource(handle, std::move(buffer));
        if (result != ResultCode::eSuccess)
        {
            return std::unexpected(result);
        }

        return { handle };
    }

    auto Device::get_image(HandleImage handle) noexcept -> std::expected<std::reference_wrapper<Image>, ResultCode>
    {
        auto result = m_images.get_resource(handle);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        auto* ptr = result.value();
        VGW_ASSERT(ptr != nullptr);
        return { *ptr };
    }

    void Device::destroy_image(HandleImage handle) noexcept
    {
        // #TODO: Handle safe deletion? Or leave to library consumer?
        auto result = m_images.set_resource(handle, nullptr);
        if (result != ResultCode::eSuccess)
        {
            return;
        }

        m_images.free_handle(handle);
    }

    auto Device::get_or_create_descriptor_set_layout(const vk::DescriptorSetLayoutCreateInfo& layoutInfo) -> vk::DescriptorSetLayout
    {
        auto setLayoutHash = std::hash<vk::DescriptorSetLayoutCreateInfo>{}(layoutInfo);
        if (m_descriptorSetLayoutMap.contains(setLayoutHash))
        {
            return m_descriptorSetLayoutMap.at(setLayoutHash).get();
        }

        m_descriptorSetLayoutMap[setLayoutHash] = m_device->createDescriptorSetLayoutUnique(layoutInfo).value;
        return m_descriptorSetLayoutMap.at(setLayoutHash).get();
    }

    auto Device::create_descriptor_sets(std::uint32_t count, const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
        -> std::vector<vk::UniqueDescriptorSet>
    {
        vk::DescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.setBindings(bindings);
        auto layout = get_or_create_descriptor_set_layout(layoutInfo);

        std::vector setLayouts(count, layout);
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.setDescriptorPool(m_descriptorPool.get());
        allocInfo.setSetLayouts(setLayouts);
        return m_device->allocateDescriptorSetsUnique(allocInfo).value;
    }

    auto Device::create_pipeline_library() -> std::unique_ptr<PipelineLibrary>
    {
        return std::make_unique<PipelineLibrary>(*this);
    }

    auto Device::create_render_pass(const RenderPassInfo& renderPassInfo) -> std::unique_ptr<RenderPass>
    {
        return std::make_unique<RenderPass>(*this, renderPassInfo);
    }

    void Device::bind_buffer(vk::DescriptorSet set,
                             std::uint32_t binding,
                             vk::DescriptorType descriptorType,
                             HandleBuffer bufferHandle,
                             std::uint64_t offset,
                             std::uint64_t range)
    {
        is_invariant();
        VGW_ASSERT(binding >= 0);
        VGW_ASSERT(descriptorType >= vk::DescriptorType::eUniformBuffer && descriptorType <= vk::DescriptorType::eStorageBufferDynamic);
        VGW_ASSERT(range >= 1)

        auto result = m_buffers.get_resource(bufferHandle);
        if (!result)
        {
            // #TODO: Handle error.
            return;
        }
        auto* bufferRef = result.value();

        m_pendingBufferInfos.push_back(std::make_unique<vk::DescriptorBufferInfo>());
        auto& bufferInfo = m_pendingBufferInfos.back();
        bufferInfo->setBuffer(bufferRef->get_buffer());
        bufferInfo->setOffset(offset);
        bufferInfo->setRange(range);

        auto& write = m_pendingDescriptorWrites.emplace_back();
        write.setDstSet(set);
        write.setDstBinding(binding);
        write.setDescriptorType(descriptorType);
        write.setDescriptorCount(1);
        write.setDstArrayElement(0);
        write.setPBufferInfo(bufferInfo.get());
    }

    void Device::flush_descriptor_writes()
    {
        is_invariant();
        if (m_pendingDescriptorWrites.empty())
        {
            return;
        }

        m_device->updateDescriptorSets(m_pendingDescriptorWrites, {});

        m_pendingBufferInfos.clear();
        m_pendingDescriptorWrites.clear();
    }

    void Device::submit(std::uint32_t queueIndex, CommandBuffer& commandBuffer, Fence* outFence)
    {
        is_invariant();
        VGW_ASSERT(queueIndex < m_queues.size());
        VGW_ASSERT(commandBuffer.is_valid());

        if (outFence != nullptr)
        {
            vk::FenceCreateInfo fenceInfo{};
            auto fence = m_device->createFence(fenceInfo).value;
            *outFence = Fence(*this, fence);
        }
        VGW_ASSERT(outFence == nullptr || outFence->is_valid());

        auto queue = m_queues.at(queueIndex);
        VGW_ASSERT(queue);

        auto cmd = commandBuffer.get_command_buffer();

        vk::SubmitInfo submitInfo{};
        submitInfo.setCommandBuffers(cmd);
        auto result = queue.submit(submitInfo, outFence ? outFence->get_fence() : nullptr);
        VGW_UNUSED(result);  // TODO: Check result.
    }

    void Device::present(std::uint32_t queueIndex, SwapChain& swapChain)
    {
        is_invariant();
        VGW_ASSERT(swapChain.is_valid());

        auto queue = m_queues.at(queueIndex);
        VGW_ASSERT(queue);

        std::vector swapChains = { swapChain.get_swap_chain() };
        const auto imageIndex = swapChain.get_image_index();
        vk::PresentInfoKHR presentInfo{};
        presentInfo.setSwapchains(swapChains);
        presentInfo.setImageIndices(imageIndex);
        presentInfo.setWaitSemaphores({});
        auto result = queue.presentKHR(presentInfo);
        VGW_UNUSED(result);  // TODO: Check result.
    }

    auto Device::operator=(Device&& rhs) noexcept -> Device&
    {
        std::swap(m_context, rhs.m_context);
        std::swap(m_physicalDevice, rhs.m_physicalDevice);
        std::swap(m_device, rhs.m_device);
        std::swap(m_allocator, rhs.m_allocator);
        std::swap(m_queues, rhs.m_queues);
        std::swap(m_descriptorPool, rhs.m_descriptorPool);
        std::swap(m_commandPoolMap, rhs.m_commandPoolMap);
        std::swap(m_descriptorSetLayoutMap, rhs.m_descriptorSetLayoutMap);
        return *this;
    }

    void Device::is_invariant() const
    {
        VGW_ASSERT(m_context);
        VGW_ASSERT(m_device);
        VGW_ASSERT(m_allocator);
        VGW_ASSERT(!m_queues.empty());
        VGW_ASSERT(m_descriptorPool);
    }

    auto Device::allocate_buffer(const BufferInfo& bufferInfo) noexcept -> std::expected<std::pair<vk::Buffer, vma::Allocation>, ResultCode>
    {
        vk::BufferCreateInfo vkBufferInfo{};
        vkBufferInfo.setSize(bufferInfo.size);
        vkBufferInfo.setUsage(bufferInfo.usage);

        vma::AllocationCreateInfo allocInfo{};
        allocInfo.setUsage(bufferInfo.memoryUsage);
        allocInfo.setFlags(bufferInfo.allocationCreateFlags);

        auto result = m_allocator->createBuffer(vkBufferInfo, allocInfo);
        if (result.result != vk::Result::eSuccess)
        {
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        auto& [buffer, allocation] = result.value;
        return { { buffer, allocation } };
    }

    auto Device::allocate_image(const ImageInfo& imageInfo) noexcept -> std::expected<std::pair<vk::Image, vma::Allocation>, ResultCode>
    {
        vk::ImageCreateInfo vkImageInfo{};
        vkImageInfo.setExtent({ imageInfo.width, imageInfo.height, imageInfo.depth });
        vkImageInfo.setMipLevels(imageInfo.mipLevels);
        vkImageInfo.setFormat(imageInfo.format);
        vkImageInfo.setUsage(imageInfo.usage);

        vma::AllocationCreateInfo allocInfo{};
        allocInfo.setUsage(vma::MemoryUsage::eAuto);

        auto result = m_allocator->createImage(vkImageInfo, allocInfo);
        if (result.result != vk::Result::eSuccess)
        {
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        auto& [image, allocation] = result.value;
        return { { image, allocation } };
    }

}