#include "vgw/pipelines.hpp"

#include "vgw/device.hpp"

#include <spirv_reflect.h>

#include <utility>

namespace VGW_NAMESPACE
{
    Pipeline::Pipeline(Device& device, vk::PipelineLayout layout, vk::PipelineBindPoint bindPoint, ShaderReflectionData reflectionData)
        : m_device(&device), m_layout(layout), m_bindPoint(bindPoint), m_reflectionData(std::move(reflectionData))
    {
    }

    Pipeline::Pipeline(Pipeline&& other) noexcept
    {
        std::swap(m_device, other.m_device);
        std::swap(m_layout, other.m_layout);
        std::swap(m_pipeline, other.m_pipeline);
        std::swap(m_bindPoint, other.m_bindPoint);
    }

    bool Pipeline::is_valid() const
    {
        return m_device && m_layout && m_device;
    }

    void Pipeline::destroy()
    {
        if (is_valid())
        {
            m_device->get_device().destroy(m_pipeline);
        }
    }

    void Pipeline::is_invariant()
    {
        VGW_ASSERT(m_device);
        VGW_ASSERT(m_layout);
        VGW_ASSERT(m_pipeline);
    }

    void Pipeline::set_layout(vk::PipelineLayout layout)
    {
        m_layout = layout;
    }

    void Pipeline::set_pipeline(vk::Pipeline pipeline)
    {
        m_pipeline = pipeline;
    }

    auto Pipeline::operator=(Pipeline&& rhs) noexcept -> Pipeline&
    {
        std::swap(m_device, rhs.m_device);
        std::swap(m_layout, rhs.m_layout);
        std::swap(m_pipeline, rhs.m_pipeline);
        std::swap(m_bindPoint, rhs.m_bindPoint);
        return *this;
    }

#pragma region Compute Pipeline

    ComputePipeline::ComputePipeline(Device& device,
                                     vk::PipelineLayout layout,
                                     vk::Pipeline pipeline,
                                     const ShaderReflectionData& reflectionData)
        : Pipeline(device, layout, vk::PipelineBindPoint::eCompute, reflectionData)
    {
    }

    ComputePipeline::ComputePipeline(ComputePipeline&& other) noexcept {}

    ComputePipeline::~ComputePipeline()
    {
        ComputePipeline::destroy();
    }

#pragma endregion

#pragma region Graphics Pipeline

    GraphicsPipeline::GraphicsPipeline(Device& device,
                                       const GraphicsPipelineInfo& pipelineInfo,
                                       vk::PipelineLayout layout,
                                       const ShaderReflectionData& reflectionData)
        : Pipeline(device, layout, vk::PipelineBindPoint::eGraphics, reflectionData)
    {
        vk::ShaderModuleCreateInfo vertex_module_info{};
        vertex_module_info.setCode(pipelineInfo.vertexCode);
        auto vertex_module = get_device()->get_device().createShaderModuleUnique(vertex_module_info);
        vk::PipelineShaderStageCreateInfo vertex_stage_info{};
        vertex_stage_info.setStage(vk::ShaderStageFlagBits::eVertex);
        vertex_stage_info.setModule(vertex_module.get());
        vertex_stage_info.setPName("main");

        vk::ShaderModuleCreateInfo fragment_module_info{};
        fragment_module_info.setCode(pipelineInfo.fragmentCode);
        auto fragment_module = get_device()->get_device().createShaderModuleUnique(fragment_module_info);
        vk::PipelineShaderStageCreateInfo fragment_stage_info{};
        fragment_stage_info.setStage(vk::ShaderStageFlagBits::eFragment);
        fragment_stage_info.setModule(fragment_module.get());
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
        rasterisation_state.setCullMode(pipelineInfo.cullMode);      // TODO: Optional.
        rasterisation_state.setFrontFace(pipelineInfo.frontFace);    // TODO: Optional.
        rasterisation_state.setLineWidth(pipelineInfo.lineWidth);    // TODO: Optional.

        vk::PipelineMultisampleStateCreateInfo multisample_state{};
        multisample_state.setRasterizationSamples(vk::SampleCountFlagBits::e1);  // #TODO: Optional.

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.setDepthTestEnable(pipelineInfo.depthTest);      // #TODO: Optional.
        depth_stencil_state.setDepthWriteEnable(pipelineInfo.depthWrite);    // #TODO: Optional.
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

        std::vector<vk::Format> colorAttachmentFormats{ vk::Format::eB8G8R8A8Srgb };  // #TODO: Optional.
        vk::PipelineRenderingCreateInfo rendering_info{};
        rendering_info.setColorAttachmentFormats(colorAttachmentFormats);
        if (pipelineInfo.depthTest)
        {
            rendering_info.setDepthAttachmentFormat(vk::Format::eD16Unorm);  // #TODO: Optional.
        }

        vk::GraphicsPipelineCreateInfo vk_pipeline_info{};
        vk_pipeline_info.setStages(stages);
        vk_pipeline_info.setLayout(get_layout());
        vk_pipeline_info.setPVertexInputState(&vertex_input_state);
        vk_pipeline_info.setPInputAssemblyState(&input_assembly_state);
        vk_pipeline_info.setPViewportState(&viewport_state);
        vk_pipeline_info.setPRasterizationState(&rasterisation_state);
        vk_pipeline_info.setPMultisampleState(&multisample_state);
        vk_pipeline_info.setPDepthStencilState(&depth_stencil_state);
        vk_pipeline_info.setPColorBlendState(&color_blend_state);
        vk_pipeline_info.setPDynamicState(&dynamic_state);
        vk_pipeline_info.setPNext(&rendering_info);

        set_pipeline(get_device()->get_device().createGraphicsPipeline({}, vk_pipeline_info).value);
    }

    GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept {}

    GraphicsPipeline::~GraphicsPipeline()
    {
        GraphicsPipeline::destroy();
    }

#pragma endregion

#pragma region Pipeline Library

    PipelineLibrary::PipelineLibrary(Device& device) : m_device(&device) {}

    auto PipelineLibrary::create_compute_pipeline(const ComputePipelineInfo& pipelineInfo) -> ComputePipeline*
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
            m_pipelineLayoutMap[layoutHash] = m_device->get_device().createPipelineLayoutUnique(layoutCreateInfo);
        }
        pipelineLayout = m_pipelineLayoutMap.at(layoutHash).get();

        std::size_t pipelineHash{ 0 };
        hash_combine(pipelineHash, layoutHash);
        hash_combine(pipelineHash, pipelineInfo.computeCode);

        if (m_computePipelineMap.contains(pipelineHash))
        {
            return m_computePipelineMap.at(pipelineHash).get();
        }

        vk::ShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.setCode(pipelineInfo.computeCode);
        auto shaderModule = m_device->get_device().createShaderModuleUnique(moduleCreateInfo);

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{};
        shaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eCompute);
        shaderStageCreateInfo.setPName("main");
        shaderStageCreateInfo.setModule(shaderModule.get());

        vk::ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.setLayout(pipelineLayout);
        pipelineCreateInfo.setStage(shaderStageCreateInfo);
        auto pipeline = m_device->get_device().createComputePipeline({}, pipelineCreateInfo).value;

        m_computePipelineMap[pipelineHash] =
            std::make_unique<ComputePipeline>(*m_device, pipelineLayout, pipeline, computeStageReflectionData);

        return m_computePipelineMap.at(pipelineHash).get();
    }

    auto PipelineLibrary::create_graphics_pipeline(const GraphicsPipelineInfo& pipelineInfo) -> GraphicsPipeline*
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
            m_pipelineLayoutMap[layoutHash] = m_device->get_device().createPipelineLayoutUnique(layoutCreateInfo);
        }
        pipelineLayout = m_pipelineLayoutMap.at(layoutHash).get();

        std::size_t pipelineHash{ 0 };
        hash_combine(pipelineHash, layoutHash);
        hash_combine(pipelineHash, pipelineInfo.vertexCode);
        hash_combine(pipelineHash, pipelineInfo.fragmentCode);
        hash_combine(pipelineHash, pipelineInfo.topology);
        hash_combine(pipelineHash, pipelineInfo.frontFace);
        hash_combine(pipelineHash, pipelineInfo.cullMode);
        hash_combine(pipelineHash, pipelineInfo.lineWidth);
        hash_combine(pipelineHash, pipelineInfo.depthTest);
        hash_combine(pipelineHash, pipelineInfo.depthWrite);

        if (m_computePipelineMap.contains(pipelineHash))
        {
            return m_graphicsPipelineMap.at(pipelineHash).get();
        }

        m_graphicsPipelineMap[pipelineHash] =
            std::make_unique<GraphicsPipeline>(*m_device, pipelineInfo, pipelineLayout, mergedReflectionData);

        return m_graphicsPipelineMap.at(pipelineHash).get();
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

#pragma endregion

}