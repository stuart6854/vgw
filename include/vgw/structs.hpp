#pragma once

#include "base.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_hash.hpp>

namespace VGW_NAMESPACE
{
    class Buffer;
    class Image;

    struct SetLayoutInfo
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings{};
    };

    struct PipelineLayoutInfo
    {
        std::vector<vk::DescriptorSetLayout> setLayouts{};
        vk::PushConstantRange constantRange{};
    };

    struct ComputePipelineInfo
    {
        vk::PipelineLayout pipelineLayout;
        std::vector<std::uint32_t> computeCode;
    };

    struct GraphicsPipelineInfo
    {
        vk::PipelineLayout pipelineLayout;
        std::vector<std::uint32_t> vertexCode;
        std::vector<std::uint32_t> fragmentCode;

        vk::PrimitiveTopology topology;
        vk::FrontFace frontFace;
        vk::CullModeFlags cullMode{ vk::CullModeFlagBits::eNone };
        float lineWidth{ 1.0f };
        bool depthTest;
        bool depthWrite;

        std::vector<vk::Format> colorAttachmentFormats;
        vk::Format depthStencilFormat;
    };

    struct SamplerInfo
    {
        vk::SamplerAddressMode addressModeU{ vk::SamplerAddressMode::eRepeat };
        vk::SamplerAddressMode addressModeV{ vk::SamplerAddressMode::eRepeat };
        vk::SamplerAddressMode addressModeW{ vk::SamplerAddressMode::eRepeat };
        vk::Filter minFilter{ vk::Filter::eLinear };
        vk::Filter magFilter{ vk::Filter::eLinear };
    };

    struct DescriptorSet
    {
        std::uint32_t set;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
    };
    struct ShaderReflectionData
    {
        std::vector<vk::VertexInputAttributeDescription> inputAttributes;
        std::uint32_t inputAttributesStride;
        std::vector<DescriptorSet> descriptorSets;
        std::vector<vk::PushConstantRange> pushConstantBlocks;
    };

    struct CopyToBuffer
    {
        Buffer* srcBuffer;
        std::uint64_t srcOffset;
        Buffer* dstBuffer;
        std::uint64_t dstOffset;
        std::uint64_t size;
    };

    struct TransitionImage
    {
        vk::ImageLayout oldLayout;
        vk::ImageLayout newLayout;
        vk::AccessFlags2 srcAccess;
        vk::AccessFlags2 dstAccess;
        vk::PipelineStageFlags2 srcStage;
        vk::PipelineStageFlags2 dstStage;
        vk::ImageSubresourceRange subresourceRange;
    };
}

namespace std
{
    template <>
    struct hash<vgw::SetLayoutInfo>
    {
        std::size_t operator()(const vgw::SetLayoutInfo& setLayoutInfo) const
        {
            std::size_t seed{ 0 };
            for (const auto& binding : setLayoutInfo.bindings)
            {
                vgw::hash_combine(seed, binding);
            }
            return seed;
        }
    };

    template <>
    struct hash<vgw::PipelineLayoutInfo>
    {
        std::size_t operator()(const vgw::PipelineLayoutInfo& pipelineLayoutInfo) const
        {
            std::size_t seed{ 0 };
            for (const auto& setLayout : pipelineLayoutInfo.setLayouts)
            {
                vgw::hash_combine(seed, setLayout);
            }
            vgw::hash_combine(seed, pipelineLayoutInfo.constantRange);
            return seed;
        }
    };

    template <>
    struct hash<std::vector<vk::VertexInputAttributeDescription>>
    {
        std::size_t operator()(const std::vector<vk::VertexInputAttributeDescription>& attributes) const
        {
            std::size_t seed = attributes.size();
            for (const auto& attribute : attributes)
            {
                vgw::hash_combine(seed, attribute);
            }
            return seed;
        }
    };

    template <>
    struct hash<std::vector<vk::DescriptorSetLayoutBinding>>
    {
        std::size_t operator()(const std::vector<vk::DescriptorSetLayoutBinding>& setBindings) const
        {
            std::size_t seed = setBindings.size();
            for (const auto& binding : setBindings)
            {
                vgw::hash_combine(seed, binding);
            }
            return seed;
        }
    };

    template <>
    struct hash<std::vector<vgw::DescriptorSet>>
    {
        std::size_t operator()(const std::vector<vgw::DescriptorSet>& sets) const
        {
            std::size_t seed = sets.size();
            for (const auto& set : sets)
            {
                vgw::hash_combine(seed, set.set);
                vgw::hash_combine(seed, set.bindings);
            }
            return seed;
        }
    };

    template <>
    struct hash<std::vector<vk::PushConstantRange>>
    {
        std::size_t operator()(const std::vector<vk::PushConstantRange>& pushConstBlocks) const
        {
            std::size_t seed = pushConstBlocks.size();
            for (const auto& pushBlock : pushConstBlocks)
            {
                vgw::hash_combine(seed, pushBlock);
            }
            return seed;
        }
    };

    template <>
    struct hash<vgw::ShaderReflectionData>
    {
        std::size_t operator()(const vgw::ShaderReflectionData& reflectionData) const
        {
            std::size_t seed{ 0 };
            vgw::hash_combine(seed, reflectionData.inputAttributes);
            vgw::hash_combine(seed, reflectionData.descriptorSets);
            vgw::hash_combine(seed, reflectionData.pushConstantBlocks);
            return seed;
        }
    };

    template <>
    struct hash<std::vector<vk::Format>>
    {
        std::size_t operator()(const std::vector<vk::Format>& formats) const
        {
            std::size_t seed = formats.size();
            for (const auto& format : formats)
            {
                vgw::hash_combine(seed, format);
            }
            return seed;
        }
    };

    template <>
    struct hash<vgw::ComputePipelineInfo>
    {
        std::size_t operator()(const vgw::ComputePipelineInfo& pipelineInfo) const
        {
            std::size_t seed{ 0 };
            vgw::hash_combine(seed, pipelineInfo.pipelineLayout);
            vgw::hash_combine(seed, pipelineInfo.computeCode);
            return seed;
        }
    };

    template <>
    struct hash<vgw::GraphicsPipelineInfo>
    {
        std::size_t operator()(const vgw::GraphicsPipelineInfo& pipelineInfo) const
        {
            std::size_t seed{ 0 };
            vgw::hash_combine(seed, pipelineInfo.pipelineLayout);
            vgw::hash_combine(seed, pipelineInfo.vertexCode);
            vgw::hash_combine(seed, pipelineInfo.fragmentCode);
            vgw::hash_combine(seed, pipelineInfo.topology);
            vgw::hash_combine(seed, pipelineInfo.frontFace);
            vgw::hash_combine(seed, pipelineInfo.cullMode);
            vgw::hash_combine(seed, pipelineInfo.lineWidth);
            vgw::hash_combine(seed, pipelineInfo.depthTest);
            vgw::hash_combine(seed, pipelineInfo.depthWrite);
            vgw::hash_combine(seed, pipelineInfo.colorAttachmentFormats);
            return seed;
        }
    };

    template <>
    struct hash<vgw::SamplerInfo>
    {
        std::size_t operator()(const vgw::SamplerInfo& samplerInfo) const
        {
            std::size_t seed{ 0 };
            vgw::hash_combine(seed, samplerInfo.addressModeU);
            vgw::hash_combine(seed, samplerInfo.addressModeV);
            vgw::hash_combine(seed, samplerInfo.addressModeW);
            vgw::hash_combine(seed, samplerInfo.minFilter);
            vgw::hash_combine(seed, samplerInfo.magFilter);
            return seed;
        }
    };
}