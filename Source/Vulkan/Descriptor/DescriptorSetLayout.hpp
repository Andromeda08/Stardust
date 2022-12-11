#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "../Device.hpp"

struct DescriptorSetLayout
{
    DescriptorSetLayout& sampled_image(uint32_t b, vk::ShaderStageFlags stage_flags);

    DescriptorSetLayout& storage_image(uint32_t b, vk::ShaderStageFlags stage_flags);

    DescriptorSetLayout& uniform_buffer(uint32_t b, vk::ShaderStageFlags stage_flags);

    DescriptorSetLayout& storage_buffer(uint32_t b, vk::ShaderStageFlags stage_flags);

    DescriptorSetLayout& sampler(uint32_t b, vk::ShaderStageFlags stage_flags);

    DescriptorSetLayout& combined_image_sampler(uint32_t binding, vk::ShaderStageFlags stage_flags);

    DescriptorSetLayout& accelerator(uint32_t b, vk::ShaderStageFlags stage_flags);

    vk::DescriptorSetLayout create(const Device& device);

    static vk::DescriptorSetLayoutBinding make_binding(uint32_t binding, uint32_t count,
                                                       vk::DescriptorType type, vk::ShaderStageFlags stage_flags);

    DescriptorSetLayout& get_bindings(std::vector<vk::DescriptorSetLayoutBinding>& in)
    {
        in = this->bindings;
        return *this;
    }

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
};
