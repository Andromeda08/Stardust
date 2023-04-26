#pragma once

#include <memory>
#include <vector>
#include <RenderGraph/RenderGraph.hpp>
#include <Rendering/RTAO/RTAOParams.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Descriptors/Descriptor.hpp>
#include <Vulkan/Image/Sampler.hpp>
#include <Vulkan/Rendering/Pipeline.hpp>

namespace sd::rg {
    class RTAONode : public Node
    {
    public:
        RTAONode(const sdvk::Context& context, const sdvk::CommandBuffers& command_buffers);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void compile() override;

    private:
        void _init_inputs();

        void _init_outputs(const sdvk::CommandBuffers& command_buffers);

        void _init_kernel();

        void _update_descriptors();

        sdvk::Image& resource();

    private:
        struct Parameters
        {
            vk::Extent2D resolution   { 1280, 720 };
            float        ao_radius    {3.0f};
            int32_t      ao_samples   {16};
            float        ao_power     {2.0f};
            int32_t      max_samples  {50000};
            int32_t      cur_samples  {0};
            vk::Bool32   accumulation {0};
        } m_parameters;

        struct Kernel
        {
            static constexpr int32_t          s_group_size = 16;
            std::unique_ptr<sdvk::Descriptor> descriptor;
            sdvk::Pipeline                    pipeline;
            vk::Sampler                       sampler;
            int32_t                           frame {0};
        } m_kernel;

    public:
        // An RTAO Node expects 3 inputs:
        // [1] G-Buffer image
        // [2] Camera from a Scene (Used for sample accumulation over multiple frames)
        // [3] Top level acceleration structure
        std::vector<std::unique_ptr<Input>> m_inputs;

        // An RTAO Node has only one output:
        // [1] AO buffer image
        std::vector<std::unique_ptr<Output>> m_outputs;

        const sdvk::Context& m_context;
    };
}