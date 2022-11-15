#include "DescriptorSetLayout.hpp"

vk::DescriptorSetLayoutBinding
DescriptorSetLayout::make_binding(uint32_t binding, uint32_t count, vk::DescriptorType type,
                                  vk::ShaderStageFlags stage_flags)
{
    vk::DescriptorSetLayoutBinding b;
    b.setBinding(binding);
    b.setDescriptorCount(count);
    b.setDescriptorType(type);
    b.setStageFlags(stage_flags);
    return b;
}

DescriptorSetLayout& DescriptorSetLayout::sampled_image(uint32_t b, vk::ShaderStageFlags stage_flags)
{
    bindings.push_back(make_binding(b, 1, vk::DescriptorType::eSampledImage, stage_flags));
    return *this;
}

DescriptorSetLayout& DescriptorSetLayout::storage_image(uint32_t b, vk::ShaderStageFlags stage_flags)
{
    bindings.push_back(make_binding(b, 1, vk::DescriptorType::eStorageImage, stage_flags));
    return *this;
}

DescriptorSetLayout& DescriptorSetLayout::uniform_buffer(uint32_t b, vk::ShaderStageFlags stage_flags)
{
    bindings.push_back(make_binding(b, 1, vk::DescriptorType::eUniformBuffer, stage_flags));
    return *this;
}

DescriptorSetLayout& DescriptorSetLayout::storage_buffer(uint32_t b, vk::ShaderStageFlags stage_flags)
{
    bindings.push_back(make_binding(b, 1, vk::DescriptorType::eStorageBuffer, stage_flags));
    return *this;
}

DescriptorSetLayout& DescriptorSetLayout::sampler(uint32_t b, vk::ShaderStageFlags stage_flags)
{
    bindings.push_back(make_binding(b, 1, vk::DescriptorType::eSampler, stage_flags));
    return *this;
}

DescriptorSetLayout& DescriptorSetLayout::accelerator(uint32_t b, vk::ShaderStageFlags stage_flags)
{
    bindings.push_back(make_binding(b, 1, vk::DescriptorType::eAccelerationStructureKHR, stage_flags));
    return *this;
}

vk::DescriptorSetLayout DescriptorSetLayout::create(const Device& device) {
    vk::DescriptorSetLayoutCreateInfo create_info;
    create_info.setBindingCount(bindings.size());
    create_info.setPBindings(bindings.data());

    vk::DescriptorSetLayout layout;
    auto res = device.handle().createDescriptorSetLayout(&create_info, nullptr, &layout);
    return layout;
}
