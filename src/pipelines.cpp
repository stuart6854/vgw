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
        : Pipeline(device, layout, pipeline, vk::PipelineBindPoint::eCompute, reflectionData)
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
                                       vk::PipelineLayout layout,
                                       vk::Pipeline pipeline,
                                       const ShaderReflectionData& reflectionData)
        : Pipeline(device, layout, pipeline, vk::PipelineBindPoint::eGraphics, reflectionData)
    {
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
        return nullptr;
    }

    auto PipelineLibrary::reflect_shader_stage(const std::vector<std::uint32_t>& code, vk::ShaderStageFlagBits shaderStage)
        -> ShaderReflectionData
    {
        ShaderReflectionData outReflectionData{};

        spv_reflect::ShaderModule module(code, SPV_REFLECT_MODULE_FLAG_NO_COPY);
        SpvReflectResult result{};

        std::uint32_t varCount{ 0 };

        if (shaderStage != vk::ShaderStageFlagBits::eCompute)
        {
            result = module.EnumerateInputVariables(&varCount, nullptr);
            VGW_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
            std::vector<SpvReflectInterfaceVariable*> inputVariables(varCount);
            result = module.EnumerateInputVariables(&varCount, inputVariables.data());
            VGW_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
            for (const auto& inputVar : inputVariables)
            {
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

#pragma endregion

}