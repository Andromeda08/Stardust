#include "DescriptorBuilder.hpp"

#include <Vulkan/Utils.hpp>

namespace sdvk
{
    DescriptorBuilder& DescriptorBuilder::sampled_image(uint32_t b, vk::ShaderStageFlags stage_flags)
    {
        _bindings.push_back(make_binding(b, vk::DescriptorType::eSampledImage, stage_flags));
        return *this;
    }

    DescriptorBuilder& DescriptorBuilder::storage_image(uint32_t b, vk::ShaderStageFlags stage_flags)
    {
        _bindings.push_back(make_binding(b, vk::DescriptorType::eStorageImage, stage_flags));
        return *this;
    }

    DescriptorBuilder& DescriptorBuilder::uniform_buffer(uint32_t b, vk::ShaderStageFlags stage_flags)
    {
        _bindings.push_back(make_binding(b, vk::DescriptorType::eUniformBuffer, stage_flags));
        return *this;
    }

    DescriptorBuilder& DescriptorBuilder::storage_buffer(uint32_t b, vk::ShaderStageFlags stage_flags)
    {
        _bindings.push_back(make_binding(b, vk::DescriptorType::eStorageBuffer, stage_flags));
        return *this;
    }

    DescriptorBuilder& DescriptorBuilder::sampler(uint32_t b, vk::ShaderStageFlags stage_flags)
    {
        _bindings.push_back(make_binding(b, vk::DescriptorType::eSampler, stage_flags));
        return *this;
    }

    DescriptorBuilder& DescriptorBuilder::combined_image_sampler(uint32_t b, vk::ShaderStageFlags stage_flags)
    {
        _bindings.push_back(make_binding(b, vk::DescriptorType::eCombinedImageSampler, stage_flags));
        return *this;
    }

    DescriptorBuilder& DescriptorBuilder::accelerator(uint32_t b, vk::ShaderStageFlags stage_flags)
    {
        _bindings.push_back(make_binding(b, vk::DescriptorType::eAccelerationStructureKHR, stage_flags));
        return *this;
    }

    vk::DescriptorSetLayoutBinding
    DescriptorBuilder::make_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stage_flags)
    {
        vk::DescriptorSetLayoutBinding b;
        b.setBinding(binding);
        b.setDescriptorCount(1);
        b.setDescriptorType(type);
        b.setStageFlags(stage_flags);
        return b;
    }

    std::unique_ptr<Descriptor> DescriptorBuilder::create(const vk::Device& device, uint32_t count)
    {
        if (_bindings.empty())
        {
            throw std::runtime_error("No descriptor bindings.");
        }

        vk::DescriptorSetLayoutCreateInfo create_info;
        create_info.setBindingCount(_bindings.size());
        create_info.setPBindings(_bindings.data());

        vk::Result result = device.createDescriptorSetLayout(&create_info, nullptr, &_layout);

        if (!_layout)
        {
            throw std::runtime_error("Missing DescriptorSetLayout.");
        }

        auto r = std::make_unique<Descriptor>(_bindings, _layout, device, count);

        if (!_name.empty())
        {
            for (int32_t i = 0; i < count; i++)
            {
                std::string name = "Descriptor "  + _name + " #" + std::to_string(i);
                sdvk::util::name_vk_object(name, (uint64_t) static_cast<VkDescriptorSet>(r->set(i)), vk::ObjectType::eDescriptorSet, device);
            }
        }

        return r;
    }

}