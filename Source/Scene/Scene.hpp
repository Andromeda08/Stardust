#pragma once

#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "Mesh.hpp"
#include "../Resources/Geometry.hpp"
#include "../Vulkan/Device.hpp"
#include "../Vulkan/Swapchain.hpp"
#include "../Vulkan/Buffer/UniformBuffer.hpp"
#include "../Vulkan/Command/CommandBuffer.hpp"
#include "../Vulkan/Descriptor/DescriptorSets.hpp"
#include "../Vulkan/Descriptor/DescriptorSetLayout.hpp"
#include "../Vulkan/GraphicsPipeline/GraphicsPipelineBuilder.hpp"
#include "../Vulkan/GraphicsPipeline/GraphicsPipelineState.hpp"
#include "../Vulkan/GraphicsPipeline/RenderPass.hpp"
#include "../Vulkan/GraphicsPipeline/ShaderModule.hpp"

class Scene
{
public:
    Scene(const CommandBuffer& command_buffer,
          const Swapchain& swap_chain,
          const RenderPass& render_pass,
          const std::vector<std::string>& shader_files)
    : m_device(command_buffer.device())
    , m_command_buffer(command_buffer)
    , m_swap_chain(swap_chain)
    , m_render_pass(render_pass)
    {
       std::vector<vk::DescriptorSetLayoutBinding> bindings;
       m_dsl = DescriptorSetLayout()
           .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex)
           .get_bindings(bindings)
           .create(m_device);

        m_ds = std::make_unique<DescriptorSets>(bindings, m_dsl, m_device);

       m_ubs.resize(2);
       for (size_t i = 0; i < 2; i++)
       {
           m_ubs[i] = std::make_unique<UniformBuffer>(m_device);
           updateUniformBuffer(i);

           m_ds->update_descriptor_set(i, 0, { m_ubs[i]->handle(), 0, sizeof(UniformBufferObject) });
       }

        createPipeline(shader_files);

        Geometry* sphere = new SphereGeometry(2.5f, glm::vec3{0.5f}, 60);
        Geometry* cube   = new CubeGeometry();

        std::default_random_engine rand(static_cast<unsigned>(time(nullptr)));
        std::uniform_int_distribution<int> uniform_dist(-1 * 128, 128);
        std::uniform_real_distribution<float> uniform_float(0.0f, 1.0f);
        std::uniform_real_distribution<float> scale_mod(1.0f, 4.0f);

        std::vector<IData> m1, m2;

        for (int i = 0; i < 128; i++)
        {
            auto x = (float) uniform_dist(rand);
            auto y = (float) uniform_dist(rand);
            auto z = (float) uniform_dist(rand);

            IData id = {glm::vec3{ x, y, z }, glm::vec3{ scale_mod(rand) }, glm::vec3{ 1 }, uniform_float(rand) * 360,
                        glm::vec3{ uniform_float(rand), uniform_float(rand), uniform_float(rand) } };

            (i % 2 == 0) ? m1.push_back(id) : m2.push_back(id);
        }

        m_objects.push_back(std::make_unique<Mesh>(sphere, m1, m_command_buffer));
        m_objects.push_back(std::make_unique<Mesh>(cube, m2, m_command_buffer));
    }

    void updateUniformBuffer(size_t index)
    {
        static auto start_time = std::chrono::high_resolution_clock::now();
        auto current_time = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

        auto model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(1, 1, 0));
        auto view = glm::lookAt(glm::vec3(128, 0, 128), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
        auto proj = glm::perspective(glm::radians(45.0f), m_swap_chain.aspectRatio(), 0.1f, 2000.0f);

        UniformBufferObject ubo {
            .view_projection = proj * view,
            .model = model
        };

        m_ubs[index]->update(ubo);
    }

    void createPipeline(const std::vector<std::string>& shader_files)
    {
        int vertex_idx = -1, fragment_idx = -1;
        for (int i = 0; i < shader_files.size(); i++)
        {
            if (shader_files[i].find("vert") != std::string::npos)
                vertex_idx = i;

            if (shader_files[i].find("frag") != std::string::npos)
                fragment_idx = i;
        }

        if (vertex_idx == -1)
            throw std::runtime_error("No vertex shader specified! (.vert)");
        if (fragment_idx == -1)
            throw std::runtime_error("No fragment shader specified! (.frag)");

        auto vsh = std::make_unique<ShaderModule>(vk::ShaderStageFlagBits::eVertex,
                                                  shader_files[vertex_idx], m_device);

        auto fsh = std::make_unique<ShaderModule>(vk::ShaderStageFlagBits::eFragment,
                                                  shader_files[fragment_idx], m_device);

        vk::PipelineLayoutCreateInfo create_info;
        create_info.setSetLayoutCount(1);
        create_info.setPSetLayouts(&m_dsl);

        auto result = m_device.handle().createPipelineLayout(&create_info, nullptr, &m_pipeline_layout);

        GraphicsPipelineState gps;
        gps.add_binding_descriptions({
            { 0, sizeof(Vertex), vk::VertexInputRate::eVertex },
            { 1, sizeof(IData), vk::VertexInputRate::eInstance }
        });
        gps.add_attribute_descriptions({
            { 0, 0, vk::Format::eR32G32B32Sfloat, 0 },
            { 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) },
            { 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) },
            { 3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv) },
            { 4, 1, vk::Format::eR32G32B32Sfloat, 0 },
            { 5, 1, vk::Format::eR32G32B32Sfloat, offsetof(IData, scale) },
            { 6, 1, vk::Format::eR32G32B32Sfloat, offsetof(IData, rotation_axis) },
            { 7, 1, vk::Format::eR32Sfloat, offsetof(IData, rotation_angle) },
            { 8, 1, vk::Format::eR32G32B32Sfloat, offsetof(IData, color) }
        });

        gps.add_scissor(m_swap_chain.make_scissor());
        gps.add_viewport(m_swap_chain.make_viewport());

        GraphicsPipelineBuilder builder(m_device, m_pipeline_layout, m_render_pass, gps);
        builder.add_shader(*vsh);
        builder.add_shader(*fsh);

        m_pipeline = builder.create_pipeline();
    }

    void draw(vk::CommandBuffer cmd, size_t current_frame) const
    {
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout, 0, 1,
                               &m_ds->get_set(current_frame), 0, nullptr);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
        for (const auto& m : m_objects)
        {
            m->draw(cmd);
        }
    }

private:
    const CommandBuffer& m_command_buffer;
    const Device& m_device;
    const Swapchain& m_swap_chain;
    const RenderPass& m_render_pass;

    // Scene objects
    std::vector<std::unique_ptr<Mesh>> m_objects;

    // Descriptor Sets
    vk::DescriptorSetLayout m_dsl;
    std::unique_ptr<DescriptorSets> m_ds;

    std::vector<std::unique_ptr<UniformBuffer>> m_ubs;

    // Rendering
    vk::PipelineLayout m_pipeline_layout;
    vk::Pipeline       m_pipeline;
};