#pragma once

#include "base.hpp"
#include "structs.hpp"

#include <vulkan/vulkan.hpp>

#include <unordered_map>

namespace VGW_NAMESPACE
{
    class Device;

    class Pipeline
    {
    public:
        Pipeline() = default;
        Pipeline(Device& device, vk::PipelineLayout layout, vk::PipelineBindPoint bindPoint, ShaderReflectionData reflectionData);
        Pipeline(const Pipeline&) = delete;
        Pipeline(Pipeline&& other) noexcept;
        virtual ~Pipeline() = default;

        /* Getters */

        auto get_device() const -> Device* { return m_device; }
        auto get_layout() const -> vk::PipelineLayout { return m_layout; }
        auto get_pipeline() const -> vk::Pipeline { return m_pipeline; }
        auto get_bind_point() const -> vk::PipelineBindPoint { return m_bindPoint; }

        auto get_reflection_data() const -> const ShaderReflectionData& { return m_reflectionData; }

        /* Methods */

        virtual bool is_valid() const;

        virtual void destroy();

        /* Operators */

        auto operator=(const Pipeline&) -> Pipeline& = delete;
        auto operator=(Pipeline&& rhs) noexcept -> Pipeline&;

    protected:
        virtual void is_invariant();

        void set_layout(vk::PipelineLayout layout);
        void set_pipeline(vk::Pipeline pipeline);

    private:
        Device* m_device{ nullptr };
        vk::PipelineLayout m_layout;
        vk::Pipeline m_pipeline;
        vk::PipelineBindPoint m_bindPoint{};

        ShaderReflectionData m_reflectionData;
    };

    class ComputePipeline final : public Pipeline
    {
    public:
        ComputePipeline() = default;
        ComputePipeline(Device& device, vk::PipelineLayout layout, vk::Pipeline pipeline, const ShaderReflectionData& reflectionData);
        ComputePipeline(const ComputePipeline&) = delete;
        ComputePipeline(ComputePipeline&& other) noexcept;
        ~ComputePipeline() override;

        /* Getters */

    private:
    };

    class GraphicsPipeline final : public Pipeline
    {
    public:
        GraphicsPipeline() = default;
        GraphicsPipeline(Device& device,
                         const GraphicsPipelineInfo& pipelineInfo,
                         vk::PipelineLayout layout,
                         const ShaderReflectionData& reflectionData);
        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline(GraphicsPipeline&& other) noexcept;
        ~GraphicsPipeline() override;

        /* Getters */

    private:
    };

    /**
     * Manages pipelines and prevents duplicate pipeline creation.
     */
    class PipelineLibrary
    {
    public:
        PipelineLibrary() = default;
        explicit PipelineLibrary(Device& device);
        PipelineLibrary(const PipelineLibrary&) = delete;
        PipelineLibrary(PipelineLibrary&& other) noexcept = delete;
        ~PipelineLibrary() = default;

        /* Methods */

        auto create_compute_pipeline(const ComputePipelineInfo& pipelineInfo) -> ComputePipeline*;
        auto create_graphics_pipeline(const GraphicsPipelineInfo& pipelineInfo) -> GraphicsPipeline*;

        /* Operators */

        auto operator=(const PipelineLibrary&) -> PipelineLibrary& = delete;
        auto operator=(PipelineLibrary&& rhs) noexcept -> PipelineLibrary& = delete;

    private:
        static auto reflect_shader_stage(const std::vector<std::uint32_t>& code, vk::ShaderStageFlagBits shaderStage)
            -> ShaderReflectionData;

        static auto merge_reflection_data(const ShaderReflectionData& reflectionDataA, const ShaderReflectionData& reflectionDataB)
            -> ShaderReflectionData;

    private:
        Device* m_device{ nullptr };

        std::unordered_map<std::size_t, vk::UniquePipelineLayout> m_pipelineLayoutMap;
        std::unordered_map<std::size_t, std::unique_ptr<ComputePipeline>> m_computePipelineMap;
        std::unordered_map<std::size_t, std::unique_ptr<GraphicsPipeline>> m_graphicsPipelineMap;
    };
}
