#pragma once

#include "base.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_hash.hpp>

namespace VGW_NAMESPACE
{
    class Buffer;

    struct ComputePipelineInfo
    {
        std::vector<std::uint32_t> computeCode;
    };

    struct GraphicsPipelineInfo
    {
        std::vector<std::uint32_t> vertexCode;
        std::vector<std::uint32_t> fragmentCode;

        vk::PrimitiveTopology topology;
        vk::FrontFace frontFace;
        vk::CullModeFlags cullMode{ vk::CullModeFlagBits::eNone };
        float lineWidth{ 1.0f };
        bool depthTest;
        bool depthWrite;
    };

    struct DescriptorSet
    {
        std::uint32_t set;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
    };
    struct ShaderReflectionData
    {
        std::vector<vk::VertexInputAttributeDescription> inputAttributes;
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
}

namespace std
{
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
}