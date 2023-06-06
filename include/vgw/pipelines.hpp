#pragma once

#include "base.hpp"
#include "handles.hpp"
#include "structs.hpp"
#include "resource_storage.hpp"

#include <vulkan/vulkan.hpp>

#include <unordered_map>

namespace VGW_NAMESPACE
{
    class Device;

    class Pipeline
    {
    public:
        Pipeline(Device& device,
                 vk::PipelineLayout layout,
                 vk::Pipeline pipeline,
                 vk::PipelineBindPoint bindPoint,
                 ShaderReflectionData reflectionData);
        Pipeline(const Pipeline&) = delete;
        Pipeline(Pipeline&& other) noexcept;
        ~Pipeline();

        /* Getters */

        auto get_device() const noexcept -> Device* { return m_device; }
        auto get_layout() const noexcept -> vk::PipelineLayout { return m_layout; }
        auto get_pipeline() const noexcept -> vk::Pipeline { return m_pipeline; }
        auto get_bind_point() const noexcept -> vk::PipelineBindPoint { return m_bindPoint; }

        auto get_reflection_data() const noexcept -> const ShaderReflectionData& { return m_reflectionData; }

        /* Operators */

        auto operator=(const Pipeline&) -> Pipeline& = delete;
        auto operator=(Pipeline&& rhs) noexcept -> Pipeline&;

    protected:
        void is_invariant() const noexcept;

    private:
        Device* m_device{ nullptr };
        vk::PipelineLayout m_layout;
        vk::Pipeline m_pipeline;
        vk::PipelineBindPoint m_bindPoint{};

        ShaderReflectionData m_reflectionData;
    };

    /**
     * Manages pipelines and prevents duplicate pipeline creation.
     */
    class PipelineLibrary
    {
    public:
        explicit PipelineLibrary(Device& device);
        PipelineLibrary(const PipelineLibrary&) = delete;
        PipelineLibrary(PipelineLibrary&& other) noexcept = delete;
        ~PipelineLibrary() = default;

        /* Methods */

        [[nodiscard]] auto create_compute_pipeline(const ComputePipelineInfo& pipelineInfo) noexcept
            -> std::expected<HandlePipeline, ResultCode>;
        [[nodiscard]] auto create_graphics_pipeline(const GraphicsPipelineInfo& pipelineInfo) noexcept
            -> std::expected<HandlePipeline, ResultCode>;

        [[nodiscard]] auto get_pipeline(HandlePipeline handle) noexcept -> std::expected<std::reference_wrapper<Pipeline>, ResultCode>;

        void destroy_pipeline(HandlePipeline handle) noexcept;

        /* Operators */

        auto operator=(const PipelineLibrary&) -> PipelineLibrary& = delete;
        auto operator=(PipelineLibrary&& rhs) noexcept -> PipelineLibrary& = delete;

    private:
        static auto reflect_shader_stage(const std::vector<std::uint32_t>& code, vk::ShaderStageFlagBits shaderStage)
            -> ShaderReflectionData;

        static auto merge_reflection_data(const ShaderReflectionData& reflectionDataA, const ShaderReflectionData& reflectionDataB)
            -> ShaderReflectionData;

        auto create_compute_pipeline(vk::PipelineLayout layout, const ComputePipelineInfo& pipelineInfo)
            -> std::expected<vk::Pipeline, ResultCode>;

        auto create_graphics_pipeline(vk::PipelineLayout layout,
                                      const GraphicsPipelineInfo& pipelineInfo,
                                      const ShaderReflectionData& reflectionData) -> std::expected<vk::Pipeline, ResultCode>;

    private:
        Device* m_device{ nullptr };

        std::unordered_map<std::size_t, vk::UniquePipelineLayout> m_pipelineLayoutMap;
        std::unordered_map<std::size_t, HandlePipeline> m_computePipelineMap;
        std::unordered_map<std::size_t, HandlePipeline> m_graphicsPipelineMap;

        ResourceStorage<HandlePipeline, Pipeline> m_pipelines;
    };
}
