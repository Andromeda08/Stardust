#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Descriptors/Descriptor.hpp>

namespace sdvk
{
    struct DescriptorBuilder
    {
        #pragma region LayoutBuilder

        DescriptorBuilder& sampler(uint32_t binding, vk::ShaderStageFlags shader_stage);

        DescriptorBuilder& sampled_image(uint32_t binding, vk::ShaderStageFlags shader_stage);

        DescriptorBuilder& combined_image_sampler(uint32_t binding, vk::ShaderStageFlags shader_stage);

        DescriptorBuilder& storage_image(uint32_t binding, vk::ShaderStageFlags shader_stage);

        DescriptorBuilder& storage_buffer(uint32_t binding, vk::ShaderStageFlags shader_stage);

        DescriptorBuilder& uniform_buffer(uint32_t binding, vk::ShaderStageFlags shader_stage);

        DescriptorBuilder& accelerator(uint32_t binding, vk::ShaderStageFlags shader_stage);

        #pragma endregion

        std::unique_ptr<Descriptor> create(vk::Device const& device, uint32_t count);

    private:
        static vk::DescriptorSetLayoutBinding
        make_binding(uint32_t b, vk::DescriptorType type, vk::ShaderStageFlags shader_stage);

        std::vector<vk::DescriptorSetLayoutBinding> _bindings;
        vk::DescriptorSetLayout _layout;
    };
}