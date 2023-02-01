#pragma once

#include <memory>
#include <Rendering/AmbientOcclusion.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Descriptors/Descriptor.hpp>
#include <Vulkan/Image/Image.hpp>
#include <Vulkan/Raytracing/Tlas.hpp>
#include <Vulkan/Rendering/Pipeline.hpp>
#include "RTAOParams.hpp"

namespace sd
{
    class RTAOKernel : public AmbientOcclusion
    {
    public:
        struct Builder
        {
            Builder& with_g_buffer(std::shared_ptr<sdvk::Image> const& g_buffer)
            {
                _g_buffer = g_buffer;
                return *this;
            }

            Builder& with_tlas(std::shared_ptr<sdvk::Tlas> const& tlas)
            {
                _tlas = tlas;
                return *this;
            }

            Builder& with_shader(std::string const& path)
            {
                _shader = path;
                return *this;
            }

            std::unique_ptr<RTAOKernel> create(sdvk::CommandBuffers const& command_buffers, sdvk::Context const& context)
            {
                return std::make_unique<RTAOKernel>(_g_buffer, _tlas, _shader, command_buffers, context);
            }

        private:
            std::string _shader;
            std::shared_ptr<sdvk::Image> _g_buffer;
            std::shared_ptr<sdvk::Tlas>  _tlas;
        };

        RTAOKernel(std::shared_ptr<sdvk::Image> const& g_buffer,
                   std::shared_ptr<sdvk::Tlas> const& tlas,
                   std::string const& compute_shader,
                   sdvk::CommandBuffers const& command_buffers,
                   sdvk::Context const& context);

        void run(const glm::mat4& view_mtx, vk::CommandBuffer const& command_buffer) override;

    private:
        void create_resources();

        void create_pipeline(std::string const& compute_shader);

        void update_descriptors();

    private:
        static constexpr int32_t s_group_size = 16;

        std::unique_ptr<sdvk::Descriptor> m_ao_descriptor;
        sdvk::Pipeline m_ao_pipeline;
        vk::Sampler    m_sampler;
        RTAOParams     m_ao_params {};
        int32_t        m_frame {0};

        struct {
            vk::DescriptorImageInfo g_buffer, ao_buffer;
            vk::WriteDescriptorSetAccelerationStructureKHR tlas;
        } m_descriptor_write_cache;

        std::shared_ptr<sdvk::Image> m_g_buffer;
        std::shared_ptr<sdvk::Tlas>  m_tlas;
    };
}