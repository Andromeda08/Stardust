#pragma once

#include <memory>
#include <vector>
#include <RenderGraph/Input.hpp>
#include <RenderGraph/Node.hpp>
#include <RenderGraph/Output.hpp>
#include <RenderGraph/Scene.hpp>
#include <Rendering/RTAO/RTAOParams.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/v2/Descriptor.hpp>
#include <Vulkan/Image/Sampler.hpp>
#include <Vulkan/Rendering/Pipeline.hpp>

namespace sd::rg {

    /* Inputs:
     * [0] G-Buffer image
     * [1] Camera from a Scene (Used for sample accumulation over multiple frames)
     * [2] Top-level Acceleration Structure
     * Outputs:
     * [0] AO buffer image
     */
    class RTAONode : public Node
    {
    public:
        RTAONode(const sdvk::Context& context, const sdvk::CommandBuffers& command_buffers);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void compile() override;

        void draw() override;

    private:
        void _init_inputs();

        void _init_outputs(const sdvk::CommandBuffers& command_buffers);

        void _init_kernel();

        void _update_descriptors();

        sdvk::Image& resource();

    private:
        struct Parameters
        {
            vk::Extent2D resolution   { 1920, 1080 };
            float        ao_radius    {3.0f};
            int32_t      ao_samples   {16};
            float        ao_power     {2.0f};
            int32_t      max_samples  {50000};
            int32_t      cur_samples  {0};
            vk::Bool32   accumulation {0};
        } m_parameters;

        struct Kernel
        {
            static constexpr int32_t              s_group_size {16};
            std::unique_ptr<sdvk::Descriptor2<1>> descriptor;
            sdvk::Pipeline                        pipeline;
            vk::Sampler                           sampler;
            int32_t                               frame {0};
        } m_kernel;

        // flag used for initial image layout transitions
        bool      m_first_execute {true};
        // reference matrix to determine if camera is stationary or not
        glm::mat4 m_ref_mat = glm::mat4(1.0f);

        const sdvk::Context& m_context;
    };
}