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
#include <functional>

/* Shader: Fullscreen Quad Vertex
#version 450
layout (location = 0) out vec2 out_texCoord;
void main()
{
    out_texCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(out_texCoord * 2.0f + -1.0f, 0.0f, 1.0f);
}
*/

/* Shader: Fullscreen Quad Fragment
#version 450
layout (location = 0) in vec2 in_texCoord;
layout (location = 0) out vec4 out_fragColor;
layout (set = 0, binding = 0) uniform sampler2D u_texture;
void main()
{
    out_fragColor = vec4(texture(u_texture, in_texCoord).rgb, 1.0);
}
*/

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

#pragma region Layers& Extensions

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

        m_pipelineLibrary = std::make_unique<PipelineLibrary>(*this);

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
        std::swap(m_setLayoutMap, other.m_setLayoutMap);
        std::swap(m_pipelineLibrary, other.m_pipelineLibrary);
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
            m_device->waitIdle();

            m_images.clear();
            m_buffers.clear();
            m_pipelineLibrary.reset();
            m_renderPasses.clear();
            m_swapChains.clear();

            m_pendingDescriptorWrites.clear();
            m_pendingBufferInfos.clear();

            m_samplerMap.clear();
            m_pipelineLayoutMap.clear();
            m_setLayoutMap.clear();
            m_commandPoolMap.clear();

            m_descriptorPool.reset();
            m_queues.clear();
            m_allocator.reset();

            m_allocator.reset();
            m_device.reset();
        }
    }

#pragma region SwapChain

    auto Device::create_swap_chain(const SwapChainInfo& swapChainInfo) noexcept -> std::expected<HandleSwapChain, ResultCode>
    {
        is_invariant();

        auto handleResult = m_swapChains.allocate_handle();
        if (!handleResult)
        {
            return std::unexpected(handleResult.error());
        }
        const auto handle = handleResult.value();

        auto swapChain = std::make_unique<SwapChain>(*this, swapChainInfo);
        auto createResult = swapChain->resize(swapChainInfo.width, swapChainInfo.height, swapChainInfo.vsync);
        if (createResult != ResultCode::eSuccess)
        {
            m_swapChains.free_handle(handle);
            return std::unexpected(createResult);
        }

        auto result = m_swapChains.set_resource(handle, std::move(swapChain));
        if (result != ResultCode::eSuccess)
        {
            m_swapChains.free_handle(handle);
            return std::unexpected(result);
        }

        return { handle };
    }

    auto Device::resize_swap_chain(HandleSwapChain handle, std::uint32_t width, std::uint32_t height, bool vsync) noexcept -> ResultCode
    {
        is_invariant();

        auto getResult = m_swapChains.get_resource(handle);
        if (!getResult)
        {
            return getResult.error();
        }
        auto& swapChainRef = getResult.value();

        auto result = swapChainRef->resize(width, height, vsync);
        if (result != ResultCode::eSuccess)
        {
            return result;
        }

        return ResultCode::eSuccess;
    }

    auto Device::get_swap_chain(HandleSwapChain handle) noexcept -> std::expected<std::reference_wrapper<SwapChain>, ResultCode>
    {
        auto result = m_swapChains.get_resource(handle);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        auto* ptr = result.value();
        VGW_ASSERT(ptr != nullptr);
        return { *ptr };
    }

    void Device::destroy_swap_chain(HandleSwapChain handle) noexcept
    {
        is_invariant();

        // #TODO: Handle safe deletion? Or leave to library consumer?
        auto result = m_swapChains.set_resource(handle, nullptr);
        if (result != ResultCode::eSuccess)
        {
            return;
        }

        m_swapChains.free_handle(handle);
    }

    void Device::acquire_next_swap_chain_image(HandleSwapChain swapChainHandle, vk::UniqueSemaphore* outSemaphore) noexcept
    {
        is_invariant();

        auto result = get_swap_chain(swapChainHandle);
        if (!result)
        {
            return;
        }

        auto& swapChainRef = result.value().get();
        swapChainRef.acquire_next_image(outSemaphore);
    }

    void Device::present_swap_chain(HandleSwapChain swapChainHandle, std::uint32_t queueIndex) noexcept
    {
        is_invariant();

        auto result = get_swap_chain(swapChainHandle);
        if (!result)
        {
            return;
        }

        auto& swapChainRef = result.value().get();
        present(swapChainRef, queueIndex);
    }

#pragma endregion

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

#pragma region RenderPass

    auto Device::create_render_pass(const RenderPassInfo& renderPassInfo) noexcept -> std::expected<HandleRenderPass, ResultCode>
    {
        is_invariant();

        auto handleResult = m_renderPasses.allocate_handle();
        if (!handleResult)
        {
            return std::unexpected(handleResult.error());
        }
        const auto handle = handleResult.value();

        auto renderPass = std::make_unique<RenderPass>(*this, renderPassInfo);
        auto createResult = renderPass->resize(renderPassInfo.width, renderPassInfo.height);
        if (createResult != ResultCode::eSuccess)
        {
            m_renderPasses.free_handle(handle);
            return std::unexpected(createResult);
        }

        auto result = m_renderPasses.set_resource(handle, std::move(renderPass));
        if (result != ResultCode::eSuccess)
        {
            m_renderPasses.free_handle(handle);
            return std::unexpected(result);
        }

        return { handle };
    }

    auto Device::resize_render_pass(HandleRenderPass handle, std::uint32_t width, std::uint32_t height) noexcept -> ResultCode
    {
        is_invariant();

        auto getResult = m_renderPasses.get_resource(handle);
        if (!getResult)
        {
            return getResult.error();
        }
        auto& bufferRef = getResult.value();

        auto result = bufferRef->resize(width, height);
        if (result != ResultCode::eSuccess)
        {
            return result;
        }

        return ResultCode::eSuccess;
    }

    auto Device::get_render_pass(HandleRenderPass handle) noexcept -> std::expected<std::reference_wrapper<RenderPass>, ResultCode>
    {
        auto result = m_renderPasses.get_resource(handle);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        auto* ptr = result.value();
        VGW_ASSERT(ptr != nullptr);
        return { *ptr };
    }

    void Device::destroy_render_pass(HandleRenderPass handle) noexcept
    {
        is_invariant();

        // #TODO: Handle safe deletion? Or leave to library consumer?
        auto result = m_renderPasses.set_resource(handle, nullptr);
        if (result != ResultCode::eSuccess)
        {
            return;
        }

        m_renderPasses.free_handle(handle);
    }

    auto Device::get_render_pass_color_image(HandleRenderPass handle, std::uint32_t imageIndex) noexcept
        -> std::expected<HandleImage, ResultCode>
    {
        auto getResult = get_render_pass(handle);
        if (!getResult)
        {
            return std::unexpected(getResult.error());
        }
        auto& renderPassRef = getResult.value().get();

        return renderPassRef.get_color_image(imageIndex);
    }

#pragma endregion

#pragma region Descriptor Sets

    auto Device::get_or_create_set_layout(const SetLayoutInfo& layoutInfo) -> std::expected<vk::DescriptorSetLayout, ResultCode>
    {
        auto setLayoutHash = std::hash<SetLayoutInfo>{}(layoutInfo);
        if (m_setLayoutMap.contains(setLayoutHash))
        {
            return m_setLayoutMap.at(setLayoutHash).get();
        }

        vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.setBindings(layoutInfo.bindings);
        auto result = m_device->createDescriptorSetLayoutUnique(layoutCreateInfo);
        if (result.result != vk::Result::eSuccess)
        {
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        m_setLayoutMap[setLayoutHash] = std::move(result.value);

        return m_setLayoutMap.at(setLayoutHash).get();
    }

    auto Device::create_descriptor_sets(std::uint32_t count, vk::DescriptorSetLayout setLayout) -> std::vector<vk::UniqueDescriptorSet>
    {
        std::vector setLayouts(count, setLayout);
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.setDescriptorPool(m_descriptorPool.get());
        allocInfo.setSetLayouts(setLayouts);
        return m_device->allocateDescriptorSetsUnique(allocInfo).value;
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

    void Device::bind_image(vk::DescriptorSet set,
                            std::uint32_t binding,
                            vk::DescriptorType descriptorType,
                            HandleImage imageHandle,
                            const ImageViewInfo& viewInfo,
                            vk::Sampler sampler)
    {
        is_invariant();
        VGW_ASSERT(binding >= 0);
        VGW_ASSERT(descriptorType >= vk::DescriptorType::eCombinedImageSampler &&
                   descriptorType <= vk::DescriptorType::eCombinedImageSampler);

        auto result = get_image(imageHandle);
        if (!result)
        {
            // #TODO: Handle error.
            return;
        }
        auto& imageRef = result.value().get();

        m_pendingImageInfos.push_back(std::make_unique<vk::DescriptorImageInfo>());
        auto& imageInfo = m_pendingImageInfos.back();
        imageInfo->setImageView(imageRef.get_view(viewInfo));
        imageInfo->setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        imageInfo->setSampler(sampler);

        auto& write = m_pendingDescriptorWrites.emplace_back();
        write.setDstSet(set);
        write.setDstBinding(binding);
        write.setDescriptorType(descriptorType);
        write.setDescriptorCount(1);
        write.setDstArrayElement(0);
        write.setPImageInfo(imageInfo.get());
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

#pragma endregion

#pragma region Pipelines

    auto Device::get_or_create_pipeline_layout(const PipelineLayoutInfo& layoutInfo) noexcept
        -> std::expected<vk::PipelineLayout, ResultCode>
    {
        const auto layoutHash = std::hash<PipelineLayoutInfo>{}(layoutInfo);
        if (m_pipelineLayoutMap.contains(layoutHash))
        {
            return { m_pipelineLayoutMap.at(layoutHash).get() };
        }

        vk::PipelineLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.setSetLayouts(layoutInfo.setLayouts);
        if (layoutInfo.constantRange.size > 0)
        {
            layoutCreateInfo.setPushConstantRanges(layoutInfo.constantRange);
        }
        auto result = m_device->createPipelineLayoutUnique(layoutCreateInfo);
        if (result.result != vk::Result::eSuccess)
        {
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        m_pipelineLayoutMap[layoutHash] = std::move(result.value);

        return { m_pipelineLayoutMap.at(layoutHash).get() };
    }

    auto Device::create_compute_pipeline(const ComputePipelineInfo& pipelineInfo) noexcept -> std::expected<HandlePipeline, ResultCode>
    {
        is_invariant();

        return m_pipelineLibrary->create_compute_pipeline(pipelineInfo);
    }

    auto Device::create_graphics_pipeline(const GraphicsPipelineInfo& pipelineInfo) noexcept -> std::expected<HandlePipeline, ResultCode>
    {
        is_invariant();

        return m_pipelineLibrary->create_graphics_pipeline(pipelineInfo);
    }

    auto Device::get_fullscreen_quad_pipeline(vk::Format colorAttachmentFormat) noexcept -> std::expected<HandlePipeline, ResultCode>
    {
        static const std::vector<std::uint32_t> VertexCode = {
            0x07230203, 0x00010000, 0x0008000b, 0x0000002c, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001, 0x4c534c47,
            0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0008000f, 0x00000000, 0x00000004, 0x6e69616d,
            0x00000000, 0x00000009, 0x0000000c, 0x0000001d, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
            0x00000000, 0x00060005, 0x00000009, 0x5f74756f, 0x43786574, 0x64726f6f, 0x00000000, 0x00060005, 0x0000000c, 0x565f6c67,
            0x65747265, 0x646e4978, 0x00007865, 0x00060005, 0x0000001b, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006,
            0x0000001b, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00070006, 0x0000001b, 0x00000001, 0x505f6c67, 0x746e696f,
            0x657a6953, 0x00000000, 0x00070006, 0x0000001b, 0x00000002, 0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e, 0x00070006,
            0x0000001b, 0x00000003, 0x435f6c67, 0x446c6c75, 0x61747369, 0x0065636e, 0x00030005, 0x0000001d, 0x00000000, 0x00040047,
            0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000c, 0x0000000b, 0x0000002a, 0x00050048, 0x0000001b, 0x00000000,
            0x0000000b, 0x00000000, 0x00050048, 0x0000001b, 0x00000001, 0x0000000b, 0x00000001, 0x00050048, 0x0000001b, 0x00000002,
            0x0000000b, 0x00000003, 0x00050048, 0x0000001b, 0x00000003, 0x0000000b, 0x00000004, 0x00030047, 0x0000001b, 0x00000002,
            0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007,
            0x00000006, 0x00000002, 0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003,
            0x00040015, 0x0000000a, 0x00000020, 0x00000001, 0x00040020, 0x0000000b, 0x00000001, 0x0000000a, 0x0004003b, 0x0000000b,
            0x0000000c, 0x00000001, 0x0004002b, 0x0000000a, 0x0000000e, 0x00000001, 0x0004002b, 0x0000000a, 0x00000010, 0x00000002,
            0x00040017, 0x00000017, 0x00000006, 0x00000004, 0x00040015, 0x00000018, 0x00000020, 0x00000000, 0x0004002b, 0x00000018,
            0x00000019, 0x00000001, 0x0004001c, 0x0000001a, 0x00000006, 0x00000019, 0x0006001e, 0x0000001b, 0x00000017, 0x00000006,
            0x0000001a, 0x0000001a, 0x00040020, 0x0000001c, 0x00000003, 0x0000001b, 0x0004003b, 0x0000001c, 0x0000001d, 0x00000003,
            0x0004002b, 0x0000000a, 0x0000001e, 0x00000000, 0x0004002b, 0x00000006, 0x00000020, 0x40000000, 0x0004002b, 0x00000006,
            0x00000022, 0xbf800000, 0x0004002b, 0x00000006, 0x00000025, 0x00000000, 0x0004002b, 0x00000006, 0x00000026, 0x3f800000,
            0x00040020, 0x0000002a, 0x00000003, 0x00000017, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8,
            0x00000005, 0x0004003d, 0x0000000a, 0x0000000d, 0x0000000c, 0x000500c4, 0x0000000a, 0x0000000f, 0x0000000d, 0x0000000e,
            0x000500c7, 0x0000000a, 0x00000011, 0x0000000f, 0x00000010, 0x0004006f, 0x00000006, 0x00000012, 0x00000011, 0x0004003d,
            0x0000000a, 0x00000013, 0x0000000c, 0x000500c7, 0x0000000a, 0x00000014, 0x00000013, 0x00000010, 0x0004006f, 0x00000006,
            0x00000015, 0x00000014, 0x00050050, 0x00000007, 0x00000016, 0x00000012, 0x00000015, 0x0003003e, 0x00000009, 0x00000016,
            0x0004003d, 0x00000007, 0x0000001f, 0x00000009, 0x0005008e, 0x00000007, 0x00000021, 0x0000001f, 0x00000020, 0x00050050,
            0x00000007, 0x00000023, 0x00000022, 0x00000022, 0x00050081, 0x00000007, 0x00000024, 0x00000021, 0x00000023, 0x00050051,
            0x00000006, 0x00000027, 0x00000024, 0x00000000, 0x00050051, 0x00000006, 0x00000028, 0x00000024, 0x00000001, 0x00070050,
            0x00000017, 0x00000029, 0x00000027, 0x00000028, 0x00000025, 0x00000026, 0x00050041, 0x0000002a, 0x0000002b, 0x0000001d,
            0x0000001e, 0x0003003e, 0x0000002b, 0x00000029, 0x000100fd, 0x00010038
        };
        static const std::vector<std::uint32_t> FragmentCode = {
            0x07230203, 0x00010000, 0x0008000b, 0x0000001b, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001, 0x4c534c47,
            0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0007000f, 0x00000004, 0x00000004, 0x6e69616d,
            0x00000000, 0x00000009, 0x00000011, 0x00030010, 0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005,
            0x00000004, 0x6e69616d, 0x00000000, 0x00060005, 0x00000009, 0x5f74756f, 0x67617266, 0x6f6c6f43, 0x00000072, 0x00050005,
            0x0000000d, 0x65745f75, 0x72757478, 0x00000065, 0x00050005, 0x00000011, 0x745f6e69, 0x6f437865, 0x0064726f, 0x00040047,
            0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000d, 0x00000022, 0x00000000, 0x00040047, 0x0000000d, 0x00000021,
            0x00000000, 0x00040047, 0x00000011, 0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002,
            0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003,
            0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00090019, 0x0000000a, 0x00000006, 0x00000001, 0x00000000,
            0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x0003001b, 0x0000000b, 0x0000000a, 0x00040020, 0x0000000c, 0x00000000,
            0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d, 0x00000000, 0x00040017, 0x0000000f, 0x00000006, 0x00000002, 0x00040020,
            0x00000010, 0x00000001, 0x0000000f, 0x0004003b, 0x00000010, 0x00000011, 0x00000001, 0x00040017, 0x00000014, 0x00000006,
            0x00000003, 0x0004002b, 0x00000006, 0x00000016, 0x3f800000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003,
            0x000200f8, 0x00000005, 0x0004003d, 0x0000000b, 0x0000000e, 0x0000000d, 0x0004003d, 0x0000000f, 0x00000012, 0x00000011,
            0x00050057, 0x00000007, 0x00000013, 0x0000000e, 0x00000012, 0x0008004f, 0x00000014, 0x00000015, 0x00000013, 0x00000013,
            0x00000000, 0x00000001, 0x00000002, 0x00050051, 0x00000006, 0x00000017, 0x00000015, 0x00000000, 0x00050051, 0x00000006,
            0x00000018, 0x00000015, 0x00000001, 0x00050051, 0x00000006, 0x00000019, 0x00000015, 0x00000002, 0x00070050, 0x00000007,
            0x0000001a, 0x00000017, 0x00000018, 0x00000019, 0x00000016, 0x0003003e, 0x00000009, 0x0000001a, 0x000100fd, 0x00010038
        };

        GraphicsPipelineInfo pipelineInfo{
            .vertexCode = VertexCode,
            .fragmentCode = FragmentCode,
            .topology = vk::PrimitiveTopology::eTriangleList,
            .frontFace = vk::FrontFace::eCounterClockwise,
            .cullMode = vk::CullModeFlagBits::eBack,
            .lineWidth = 1.0f,
            .depthTest = false,
            .depthWrite = false,
            .colorAttachmentFormats = { colorAttachmentFormat },
        };
        return create_graphics_pipeline(pipelineInfo);
    }

    auto Device::get_pipeline(HandlePipeline handle) noexcept -> std::expected<std::reference_wrapper<Pipeline>, ResultCode>
    {
        return m_pipelineLibrary->get_pipeline(handle);
    }

    void Device::destroy_pipeline(HandlePipeline handle) noexcept
    {
        m_pipelineLibrary->destroy_pipeline(handle);
    }

#pragma endregion

#pragma region Buffers

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
            m_buffers.free_handle(handle);
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        auto [vkBuffer, vmaAllocation] = createResult.value();
        auto buffer = std::make_unique<Buffer>(*this, vkBuffer, vmaAllocation, bufferInfo);

        auto result = m_buffers.set_resource(handle, std::move(buffer));
        if (result != ResultCode::eSuccess)
        {
            m_buffers.free_handle(handle);
            return std::unexpected(result);
        }

        return { handle };
    }

    auto Device::resize_buffer(HandleBuffer handle, std::uint64_t size) noexcept -> ResultCode
    {
        is_invariant();

        auto getResult = m_buffers.get_resource(handle);
        if (!getResult)
        {
            return getResult.error();
        }
        auto& bufferRef = getResult.value();

        auto bufferInfo = bufferRef->get_info();
        bufferInfo.size = size;
        auto createResult = allocate_buffer(bufferInfo);
        if (!createResult)
        {
            return ResultCode::eFailedToCreate;
        }

        auto [vkBuffer, vmaAllocation] = createResult.value();
        auto buffer = std::make_unique<Buffer>(*this, vkBuffer, vmaAllocation, bufferInfo);

        auto result = m_buffers.set_resource(handle, std::move(buffer));
        if (result != ResultCode::eSuccess)
        {
            return result;
        }

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

#pragma endregion

#pragma region Images

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
            m_images.free_handle(handle);
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        auto [vkImage, vmaAllocation] = createResult.value();
        auto buffer = std::make_unique<Image>(*this, vkImage, vmaAllocation, imageInfo);

        auto result = m_images.set_resource(handle, std::move(buffer));
        if (result != ResultCode::eSuccess)
        {
            m_images.free_handle(handle);
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

#pragma endregion

    auto Device::get_or_create_sampler(const SamplerInfo& samplerInfo) noexcept -> std::expected<vk::Sampler, ResultCode>
    {
        const auto samplerHash = std::hash<SamplerInfo>{}(samplerInfo);
        if (m_samplerMap.contains(samplerHash))
        {
            return { m_samplerMap.at(samplerHash).get() };
        }

        vk::SamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.setAddressModeU(samplerInfo.addressModeU);
        samplerCreateInfo.setAddressModeV(samplerInfo.addressModeV);
        samplerCreateInfo.setAddressModeW(samplerInfo.addressModeW);
        samplerCreateInfo.setMinFilter(samplerInfo.minFilter);
        samplerCreateInfo.setMagFilter(samplerInfo.magFilter);
        auto result = m_device->createSamplerUnique(samplerCreateInfo);
        if (result.result != vk::Result::eSuccess)
        {
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        m_samplerMap[samplerHash] = std::move(result.value);

        return { m_samplerMap.at(samplerHash).get() };
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

    void Device::present(SwapChain& swapChain, std::uint32_t queueIndex)
    {
        is_invariant();

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
        std::swap(m_setLayoutMap, rhs.m_setLayoutMap);
        std::swap(m_pipelineLibrary, rhs.m_pipelineLibrary);
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
        vk::ImageType type{};
        if (imageInfo.depth > 1)
        {
            type = vk::ImageType::e3D;
        }
        else if (imageInfo.height > 1)
        {
            type = vk::ImageType::e2D;
        }
        else
        {
            type = vk::ImageType::e1D;
        }

        vk::ImageCreateInfo vkImageInfo{};
        vkImageInfo.setImageType(type);
        vkImageInfo.setFormat(imageInfo.format);
        vkImageInfo.setExtent({ imageInfo.width, imageInfo.height, imageInfo.depth });
        vkImageInfo.setMipLevels(imageInfo.mipLevels);
        vkImageInfo.setArrayLayers(1);                        // #TODO: Support array images.
        vkImageInfo.setSamples(vk::SampleCountFlagBits::e1);  // #TODO: Support image samples.
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