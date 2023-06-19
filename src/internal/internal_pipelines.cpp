#include "internal_pipelines.hpp"

#include "internal_device.hpp"

namespace vgw::internal
{
    auto internal_pipeline_compute_create(const ComputePipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        vk::ShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.setCode(pipelineInfo.computeCode);
        auto shaderModuleResult = deviceRef.device.createShaderModuleUnique(moduleCreateInfo);
        if (shaderModuleResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::ShaderModule!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        const auto& shaderModule = shaderModuleResult.value;

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{};
        shaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eCompute);
        shaderStageCreateInfo.setPName("main");
        shaderStageCreateInfo.setModule(shaderModule.get());

        vk::ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.setLayout(pipelineInfo.layout);
        pipelineCreateInfo.setStage(shaderStageCreateInfo);
        auto createResult = deviceRef.device.createComputePipeline({}, pipelineCreateInfo);
        if (createResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::Pipeline (Compute)!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        deviceRef.pipelineMap[createResult.value] = PipelineData{
            pipelineInfo.layout,
            createResult.value,
            vk::PipelineBindPoint::eCompute,
        };
        return createResult.value;
    }

    auto internal_pipeline_graphics_create(const GraphicsPipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        vk::ShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.setCode(pipelineInfo.vertexCode);
        auto shaderModuleResult = deviceRef.device.createShaderModuleUnique(moduleCreateInfo);
        if (shaderModuleResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::ShaderModule!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        const auto vertexShaderModule = std::move(shaderModuleResult.value);

        moduleCreateInfo.setCode(pipelineInfo.fragmentCode);
        shaderModuleResult = deviceRef.device.createShaderModuleUnique(moduleCreateInfo);
        if (shaderModuleResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::ShaderModule!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        const auto fragmentShaderModule = std::move(shaderModuleResult.value);

        vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo{};
        vertexShaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eVertex);
        vertexShaderStageCreateInfo.setPName("main");
        vertexShaderStageCreateInfo.setModule(vertexShaderModule.get());

        vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{};
        fragmentShaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eFragment);
        fragmentShaderStageCreateInfo.setPName("main");
        fragmentShaderStageCreateInfo.setModule(fragmentShaderModule.get());

        const std::vector stages = { vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo };

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
        //        vk_binding.setStride(reflectionData.inputAttributesStride);

        vk::PipelineVertexInputStateCreateInfo vertex_input_state{};
        //        if (!reflectionData.inputAttributes.empty())
        //        {
        //            vertex_input_state.setVertexAttributeDescriptions(reflectionData.inputAttributes);
        //            vertex_input_state.setVertexBindingDescriptions(vk_binding);
        //        }

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

        vk::GraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.setStages(stages);
        pipelineCreateInfo.setLayout(pipelineInfo.layout);
        pipelineCreateInfo.setPVertexInputState(&vertex_input_state);
        pipelineCreateInfo.setPInputAssemblyState(&input_assembly_state);
        pipelineCreateInfo.setPViewportState(&viewport_state);
        pipelineCreateInfo.setPRasterizationState(&rasterisation_state);
        pipelineCreateInfo.setPMultisampleState(&multisample_state);
        pipelineCreateInfo.setPDepthStencilState(&depth_stencil_state);
        pipelineCreateInfo.setPColorBlendState(&color_blend_state);
        pipelineCreateInfo.setPDynamicState(&dynamic_state);
        pipelineCreateInfo.setPNext(&rendering_info);
        auto createResult = deviceRef.device.createGraphicsPipeline({}, pipelineCreateInfo);
        if (createResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::Pipeline (Graphics)!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        deviceRef.pipelineMap[createResult.value] = PipelineData{
            pipelineInfo.layout,
            createResult.value,
            vk::PipelineBindPoint::eGraphics,
        };
        return createResult.value;
    }

    auto internal_pipeline_get(vk::Pipeline pipeline) -> std::expected<std::reference_wrapper<PipelineData>, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        const auto it = deviceRef.pipelineMap.find(pipeline);
        if (it != deviceRef.pipelineMap.end())
        {
            auto pipelineRef = std::ref(it->second);
            return pipelineRef;
        }

        return std::unexpected(ResultCode::eInvalidHandle);
    }

    void internal_pipeline_bind(vk::CommandBuffer cmdBuffer, vk::Pipeline pipeline)
    {
        auto getResult = internal_pipeline_get(pipeline);
        if (!getResult)
        {
            log_error("Failed to get pipeline!");
            return;
        }
        auto& pipelineRef = getResult.value().get();

        cmdBuffer.bindPipeline(pipelineRef.bindPoint, pipelineRef.pipeline);
    }
}