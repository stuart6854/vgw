#include "vgw/pipelines.hpp"

#include "vgw/device.hpp"

#include <spirv_reflect.h>

#include <utility>

namespace VGW_NAMESPACE
{
    Pipeline::Pipeline(Device& device,
                       vk::PipelineLayout layout,
                       vk::Pipeline pipeline,
                       vk::PipelineBindPoint bindPoint,
                       ShaderReflectionData reflectionData)
        : m_device(&device), m_layout(layout), m_pipeline(pipeline), m_bindPoint(bindPoint), m_reflectionData(std::move(reflectionData))
    {
        is_invariant();
    }

    Pipeline::Pipeline(Pipeline&& other) noexcept
    {
        std::swap(m_device, other.m_device);
        std::swap(m_layout, other.m_layout);
        std::swap(m_pipeline, other.m_pipeline);
        std::swap(m_bindPoint, other.m_bindPoint);
    }

    Pipeline::~Pipeline()
    {
        m_device->get_device().destroy(m_pipeline);
    }

    void Pipeline::is_invariant() const noexcept
    {
        VGW_ASSERT(m_device);
        VGW_ASSERT(m_layout);
        VGW_ASSERT(m_pipeline);
    }

    auto Pipeline::operator=(Pipeline&& rhs) noexcept -> Pipeline&
    {
        std::swap(m_device, rhs.m_device);
        std::swap(m_layout, rhs.m_layout);
        std::swap(m_pipeline, rhs.m_pipeline);
        std::swap(m_bindPoint, rhs.m_bindPoint);
        return *this;
    }

#pragma region Pipeline Library

    PipelineLibrary::PipelineLibrary(Device& device) : m_device(&device) {}

    auto PipelineLibrary::create_compute_pipeline(const ComputePipelineInfo& pipelineInfo) noexcept
        -> std::expected<HandlePipeline, ResultCode>
    {
        auto computeStageReflectionData = reflect_shader_stage(pipelineInfo.computeCode, vk::ShaderStageFlagBits::eCompute);

        std::size_t layoutHash{ 0 };
        hash_combine(layoutHash, computeStageReflectionData.descriptorSets);
        hash_combine(layoutHash, computeStageReflectionData.pushConstantBlocks);

        vk::PipelineLayout pipelineLayout{};
        if (!m_pipelineLayoutMap.contains(layoutHash))
        {
            std::vector<vk::DescriptorSetLayout> setLayouts{};
            for (const auto& descriptorSet : computeStageReflectionData.descriptorSets)
            {
                vk::DescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.setBindings(descriptorSet.bindings);
                setLayouts.emplace_back(m_device->get_or_create_descriptor_set_layout(layoutInfo));
            }

            vk::PipelineLayoutCreateInfo layoutCreateInfo{};
            layoutCreateInfo.setSetLayouts(setLayouts);
            layoutCreateInfo.setPushConstantRanges(computeStageReflectionData.pushConstantBlocks);
            m_pipelineLayoutMap[layoutHash] = m_device->get_device().createPipelineLayoutUnique(layoutCreateInfo).value;
        }
        pipelineLayout = m_pipelineLayoutMap.at(layoutHash).get();

        std::size_t pipelineHash{ 0 };
        hash_combine(pipelineHash, layoutHash);
        hash_combine(pipelineHash, pipelineInfo.computeCode);
        if (m_computePipelineMap.contains(pipelineHash))
        {
            return m_computePipelineMap.at(pipelineHash);
        }

        auto handleResult = m_pipelines.allocate_handle();
        if (!handleResult)
        {
            return std::unexpected(handleResult.error());
        }
        const auto handle = handleResult.value();

        auto createResult = create_compute_pipeline(pipelineLayout, pipelineInfo);
        if (!createResult)
        {
            m_pipelines.free_handle(handle);
            return std::unexpected(createResult.error());
        }
        auto vkPipeline = createResult.value();

        auto pipeline =
            std::make_unique<Pipeline>(*m_device, pipelineLayout, vkPipeline, vk::PipelineBindPoint::eCompute, computeStageReflectionData);
        auto result = m_pipelines.set_resource(handle, std::move(pipeline));
        if (result != ResultCode::eSuccess)
        {
            m_pipelines.free_handle(handle);
            return std::unexpected(result);
        }

        return handle;
    }

    auto PipelineLibrary::create_graphics_pipeline(const GraphicsPipelineInfo& pipelineInfo) noexcept
        -> std::expected<HandlePipeline, ResultCode>
    {
        auto vertexStageReflectionData = reflect_shader_stage(pipelineInfo.vertexCode, vk::ShaderStageFlagBits::eVertex);
        auto fragmentStageReflectionData = reflect_shader_stage(pipelineInfo.fragmentCode, vk::ShaderStageFlagBits::eFragment);

        auto mergedReflectionData = merge_reflection_data(vertexStageReflectionData, fragmentStageReflectionData);

        std::size_t layoutHash{ 0 };
        hash_combine(layoutHash, mergedReflectionData.inputAttributes);
        hash_combine(layoutHash, mergedReflectionData.descriptorSets);
        hash_combine(layoutHash, mergedReflectionData.pushConstantBlocks);

        vk::PipelineLayout pipelineLayout{};
        if (!m_pipelineLayoutMap.contains(layoutHash))
        {
            std::vector<vk::DescriptorSetLayout> setLayouts{};
            for (const auto& descriptorSet : mergedReflectionData.descriptorSets)
            {
                vk::DescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.setBindings(descriptorSet.bindings);
                setLayouts.emplace_back(m_device->get_or_create_descriptor_set_layout(layoutInfo));
            }

            vk::PipelineLayoutCreateInfo layoutCreateInfo{};
            layoutCreateInfo.setSetLayouts(setLayouts);
            layoutCreateInfo.setPushConstantRanges(mergedReflectionData.pushConstantBlocks);
            m_pipelineLayoutMap[layoutHash] = m_device->get_device().createPipelineLayoutUnique(layoutCreateInfo).value;
        }
        pipelineLayout = m_pipelineLayoutMap.at(layoutHash).get();

        std::size_t pipelineHash{ 0 };
        hash_combine(pipelineHash, layoutHash);
        hash_combine(pipelineHash, pipelineInfo);
        if (m_graphicsPipelineMap.contains(pipelineHash))
        {
            return m_graphicsPipelineMap.at(pipelineHash);
        }

        auto handleResult = m_pipelines.allocate_handle();
        if (!handleResult)
        {
            return std::unexpected(handleResult.error());
        }
        const auto handle = handleResult.value();

        auto createResult = create_graphics_pipeline(pipelineLayout, pipelineInfo, mergedReflectionData);
        if (!createResult)
        {
            m_pipelines.free_handle(handle);
            return std::unexpected(createResult.error());
        }
        auto vkPipeline = createResult.value();

        auto pipeline = std::make_unique<Pipeline>(*m_device, pipelineLayout, vkPipeline, vk::PipelineBindPoint::eGraphics, mergedReflectionData);
        auto result = m_pipelines.set_resource(handle, std::move(pipeline));
        if (result != ResultCode::eSuccess)
        {
            m_pipelines.free_handle(handle);
            return std::unexpected(result);
        }

        return handle;
    }

    auto PipelineLibrary::get_pipeline(HandlePipeline handle) noexcept -> std::expected<std::reference_wrapper<Pipeline>, ResultCode>
    {
        auto result = m_pipelines.get_resource(handle);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        auto* ptr = result.value();
        VGW_ASSERT(ptr != nullptr);
        return { *ptr };
    }

    void PipelineLibrary::destroy_pipeline(HandlePipeline handle) noexcept
    {
        // #TODO: Handle safe deletion? Or leave to library consumer?
        auto result = m_pipelines.set_resource(handle, nullptr);
        if (result != ResultCode::eSuccess)
        {
            return;
        }

        m_pipelines.free_handle(handle);
    }

    auto PipelineLibrary::reflect_shader_stage(const std::vector<std::uint32_t>& code, vk::ShaderStageFlagBits shaderStage)
        -> ShaderReflectionData
    {
        ShaderReflectionData outReflectionData{};

        spv_reflect::ShaderModule module(code, SPV_REFLECT_MODULE_FLAG_NO_COPY);
        SpvReflectResult result{};

        std::uint32_t varCount{ 0 };

        if (shaderStage == vk::ShaderStageFlagBits::eVertex)
        {
            result = module.EnumerateInputVariables(&varCount, nullptr);
            VGW_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
            std::vector<SpvReflectInterfaceVariable*> inputVariables(varCount);
            result = module.EnumerateInputVariables(&varCount, inputVariables.data());
            VGW_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
            for (const auto& inputVar : inputVariables)
            {
                if (inputVar->name == nullptr)
                {
                    continue;
                }
                auto attribute = outReflectionData.inputAttributes.emplace_back();
                attribute.setBinding(0);
                attribute.setLocation(inputVar->location);
                attribute.setFormat(static_cast<vk::Format>(inputVar->format));
                attribute.setOffset(0);
            }
        }

        // Descriptor Sets
        result = module.EnumerateDescriptorSets(&varCount, nullptr);
        VGW_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
        std::vector<SpvReflectDescriptorSet*> descriptorSets(varCount);
        result = module.EnumerateDescriptorSets(&varCount, descriptorSets.data());
        VGW_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
        for (const auto& descriptorSet : descriptorSets)
        {
            auto& outSet = outReflectionData.descriptorSets.emplace_back();
            outSet.set = descriptorSet->set;
            for (auto i = 0; i < descriptorSet->binding_count; ++i)
            {
                const auto* setBinding = descriptorSet->bindings[i];
                auto& outBinding = outSet.bindings.emplace_back();
                outBinding.setBinding(setBinding->binding);
                outBinding.setDescriptorType(static_cast<vk::DescriptorType>(setBinding->descriptor_type));
                outBinding.setDescriptorCount(setBinding->count);
                outBinding.setStageFlags(shaderStage);
            }
        }

        // Push Constant Blocks
        result = module.EnumeratePushConstantBlocks(&varCount, nullptr);
        VGW_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
        std::vector<SpvReflectBlockVariable*> pushConstBlocks(varCount);
        result = module.EnumeratePushConstantBlocks(&varCount, pushConstBlocks.data());
        VGW_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
        for (const auto& pushConstBlock : pushConstBlocks)
        {
            auto& outPushBlock = outReflectionData.pushConstantBlocks.emplace_back();
            outPushBlock.setOffset(pushConstBlock->offset);
            outPushBlock.setSize(pushConstBlock->size);
            outPushBlock.setStageFlags(shaderStage);
        }

        return outReflectionData;
    }

    auto PipelineLibrary::merge_reflection_data(const ShaderReflectionData& reflectionDataA, const ShaderReflectionData& reflectionDataB)
        -> ShaderReflectionData
    {
        auto resultReflectionData = reflectionDataA;
        // #TODO: Merge reflection data
        return resultReflectionData;
    }

    auto PipelineLibrary::create_compute_pipeline(vk::PipelineLayout layout, const ComputePipelineInfo& pipelineInfo)
        -> std::expected<vk::Pipeline, ResultCode>
    {
        vk::ShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.setCode(pipelineInfo.computeCode);
        auto shaderModuleResult = m_device->get_device().createShaderModuleUnique(moduleCreateInfo);
        if (shaderModuleResult.result != vk::Result::eSuccess)
        {
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        const auto& shaderModule = shaderModuleResult.value;

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{};
        shaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eCompute);
        shaderStageCreateInfo.setPName("main");
        shaderStageCreateInfo.setModule(shaderModule.get());

        vk::ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.setLayout(layout);
        pipelineCreateInfo.setStage(shaderStageCreateInfo);
        auto createResult = m_device->get_device().createComputePipeline({}, pipelineCreateInfo);
        if (createResult.result != vk::Result::eSuccess)
        {
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        return createResult.value;
    }

    auto PipelineLibrary::create_graphics_pipeline(vk::PipelineLayout layout,
                                                   const GraphicsPipelineInfo& pipelineInfo,
                                                   const ShaderReflectionData& reflectionData) -> std::expected<vk::Pipeline, ResultCode>
    {
        vk::ShaderModuleCreateInfo vertex_module_info{};
        vertex_module_info.setCode(pipelineInfo.vertexCode);
        auto moduleResult = m_device->get_device().createShaderModuleUnique(vertex_module_info);
        if (moduleResult.result != vk::Result::eSuccess)
        {
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        const auto vertexModule = std::move(moduleResult.value);

        vk::PipelineShaderStageCreateInfo vertex_stage_info{};
        vertex_stage_info.setStage(vk::ShaderStageFlagBits::eVertex);
        vertex_stage_info.setModule(vertexModule.get());
        vertex_stage_info.setPName("main");

        vk::ShaderModuleCreateInfo fragment_module_info{};
        fragment_module_info.setCode(pipelineInfo.fragmentCode);
        moduleResult = m_device->get_device().createShaderModuleUnique(fragment_module_info);
        if (moduleResult.result != vk::Result::eSuccess)
        {
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        const auto fragmentModule = std::move(moduleResult.value);

        vk::PipelineShaderStageCreateInfo fragment_stage_info{};
        fragment_stage_info.setStage(vk::ShaderStageFlagBits::eFragment);
        fragment_stage_info.setModule(fragmentModule.get());
        fragment_stage_info.setPName("main");

        const std::vector stages = { vertex_stage_info, fragment_stage_info };

#if 0
        std::uint32_t stride{};
        std::vector<vk::VertexInputAttributeDescription> vk_attributes(pipelineInfo.a.size());
        for (auto i = 0; i < vk_attributes.size(); ++i)
        {
            const auto& attribute = graphicsPipelineInfo.vertexAttributes.at(i);
            auto& vk_attribute = vk_attributes.at(i);
            vk_attribute.setBinding(0);
            vk_attribute.setLocation(i);
            vk_attribute.setFormat(convert_format_to_vk_format(attribute.format));
            vk_attribute.setOffset(stride);

            stride += convert_format_to_byte_size(attribute.format);
        }
#endif

        vk::VertexInputBindingDescription vk_binding{};
        vk_binding.setBinding(0);
        vk_binding.setInputRate(vk::VertexInputRate::eVertex);
        vk_binding.setStride(reflectionData.inputAttributesStride);

        vk::PipelineVertexInputStateCreateInfo vertex_input_state{};
        if (!reflectionData.inputAttributes.empty())
        {
            vertex_input_state.setVertexAttributeDescriptions(reflectionData.inputAttributes);
            vertex_input_state.setVertexBindingDescriptions(vk_binding);
        }

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state{};
        input_assembly_state.setTopology(pipelineInfo.topology);

        vk::PipelineViewportStateCreateInfo viewport_state{};
        viewport_state.setViewportCount(1);
        viewport_state.setScissorCount(1);

        vk::PipelineRasterizationStateCreateInfo rasterisation_state{};
        rasterisation_state.setPolygonMode(vk::PolygonMode::eFill);  // TODO: Optional.
        rasterisation_state.setCullMode(pipelineInfo.cullMode);
        rasterisation_state.setFrontFace(pipelineInfo.frontFace);
        rasterisation_state.setLineWidth(pipelineInfo.lineWidth);

        vk::PipelineMultisampleStateCreateInfo multisample_state{};
        multisample_state.setRasterizationSamples(vk::SampleCountFlagBits::e1);  // #TODO: Optional.

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.setDepthTestEnable(pipelineInfo.depthTest);
        depth_stencil_state.setDepthWriteEnable(pipelineInfo.depthWrite);
        depth_stencil_state.setDepthCompareOp(vk::CompareOp::eLessOrEqual);  // #TODO: Optional.
        depth_stencil_state.setStencilTestEnable(false);                     // #TODO: Optional.

        std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments{ vk::PipelineColorBlendAttachmentState(
            VK_FALSE,
            vk::BlendFactor::eZero,
            vk::BlendFactor::eOne,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eZero,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eA) };
        vk::PipelineColorBlendStateCreateInfo color_blend_state{};
        color_blend_state.setAttachments(colorBlendAttachments);

        std::vector<vk::DynamicState> dynamicStates{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        vk::PipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.setDynamicStates(dynamicStates);

        vk::PipelineRenderingCreateInfo rendering_info{};
        rendering_info.setColorAttachmentFormats(pipelineInfo.colorAttachmentFormats);
        if (pipelineInfo.depthTest)
        {
            rendering_info.setDepthAttachmentFormat(pipelineInfo.depthStencilFormat);
        }

        vk::GraphicsPipelineCreateInfo vk_pipeline_info{};
        vk_pipeline_info.setStages(stages);
        vk_pipeline_info.setLayout(layout);
        vk_pipeline_info.setPVertexInputState(&vertex_input_state);
        vk_pipeline_info.setPInputAssemblyState(&input_assembly_state);
        vk_pipeline_info.setPViewportState(&viewport_state);
        vk_pipeline_info.setPRasterizationState(&rasterisation_state);
        vk_pipeline_info.setPMultisampleState(&multisample_state);
        vk_pipeline_info.setPDepthStencilState(&depth_stencil_state);
        vk_pipeline_info.setPColorBlendState(&color_blend_state);
        vk_pipeline_info.setPDynamicState(&dynamic_state);
        vk_pipeline_info.setPNext(&rendering_info);

        auto result = m_device->get_device().createGraphicsPipeline({}, vk_pipeline_info);
        if (result.result != vk::Result::eSuccess)
        {
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        return result.value;
    }

#pragma endregion

}