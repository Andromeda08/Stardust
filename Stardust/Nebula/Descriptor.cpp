#include "Descriptor.hpp"

#include <format>
#include <stdexcept>
#include <vector>

namespace Nebula
{
    vk::DescriptorType get_vk_descriptor_type(DescriptorType descriptor_type)
    {
        switch (descriptor_type)
        {
            case DescriptorType::eCombinedImageSampler:
                return vk::DescriptorType::eCombinedImageSampler;
            case DescriptorType::eStorageBuffer:
                return vk::DescriptorType::eStorageBuffer;
            case DescriptorType::eStorageImage:
                return vk::DescriptorType::eStorageImage;
            case DescriptorType::eSampledImage:
                return vk::DescriptorType::eSampledImage;
            case DescriptorType::eSampler:
                return vk::DescriptorType::eSampler;
            case DescriptorType::eUniformBuffer:
                return vk::DescriptorType::eUniformBuffer;
            case DescriptorType::eUniformBufferDynamic:
                return vk::DescriptorType::eUniformBufferDynamic;
            case DescriptorType::eAccelerationStructure:
                return vk::DescriptorType::eAccelerationStructureKHR;
            default:
                throw std::runtime_error("Unknown descriptor type.");
        }
    }
    
    Descriptor::Descriptor(uint32_t set_count,
                           const std::vector<vk::DescriptorSetLayoutBinding>& bindings,
                           const sdvk::Context& context,
                           const std::string& debug_name)
    : m_context(context), m_bindings(bindings), m_name(debug_name), m_set_count(set_count)
    {
        _create_pool();
        _create_layout();
        _create_descriptors();

        if (!debug_name.empty())
        {
            // TODO: Name objs.
        }
    }

    void Descriptor::_create_pool()
    {
        std::vector<vk::DescriptorPoolSize> pool_sizes;
        pool_sizes.reserve(m_bindings.size());

        for (const auto& binding : m_bindings)
        {
            pool_sizes.emplace_back(binding.descriptorType, binding.descriptorCount);
        }

        vk::DescriptorPoolCreateInfo create_info;
        create_info.setMaxSets(m_set_count);
        create_info.setPoolSizeCount(static_cast<uint32_t>(pool_sizes.size()));
        create_info.setPPoolSizes(pool_sizes.data());
        auto result = m_context.device().createDescriptorPool(&create_info, nullptr, &m_pool);

        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create DescriptorPool.");
        }
    }

    void Descriptor::_create_layout()
    {
        vk::DescriptorSetLayoutCreateInfo create_info;
        create_info.setBindingCount(static_cast<uint32_t>(m_bindings.size()));
        create_info.setPBindings(m_bindings.data());
        auto result = m_context.device().createDescriptorSetLayout(&create_info, nullptr, &m_layout);

        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create DescriptorSetLayout.");
        }
    }

    void Descriptor::_create_descriptors()
    {
        m_descriptors.resize(m_set_count);
        std::vector<vk::DescriptorSetLayout> layouts(m_set_count, m_layout);
        vk::DescriptorSetAllocateInfo alloc_info;
        alloc_info.setDescriptorPool(m_pool);
        alloc_info.setDescriptorSetCount(m_set_count);
        alloc_info.setPSetLayouts(layouts.data());
        auto result = m_context.device().allocateDescriptorSets(&alloc_info, m_descriptors.data());

        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error(std::format("Failed to allocate {} DescriptorSets.", std::to_string(m_set_count)));
        }
    }

    const vk::DescriptorSet& Descriptor::set(uint32_t index) const
    {
        if (index > m_descriptors.size())
        {
            throw std::out_of_range(std::format("Index {} out of range for descriptor sets.", std::to_string(index)));
        }

        return m_descriptors[index];
    }

    const vk::DescriptorSet& Descriptor::operator[](uint32_t index) const
    {
        return this->set(index);
    }

    const vk::DescriptorSetLayout& Descriptor::layout() const
    {
        return m_layout;
    }

    uint32_t Descriptor::set_count() const
    {
        return static_cast<uint32_t>(m_descriptors.size());
    }

    Descriptor::Write Descriptor::begin_write(uint32_t set_index)
    {
        return Descriptor::Write(*this, set_index, m_context);
    }

    // Writes

    Descriptor::Write::Write(const Descriptor& descriptor, uint32_t set_index, const sdvk::Context& context)
    : _context(context), _descriptor(descriptor), _set_index(set_index)
    {
    }
    
    Descriptor::Write& Descriptor::Write::acceleration_structure(uint32_t binding,
                                                                 uint32_t acceleration_structure_count,
                                                                 const vk::AccelerationStructureKHR* p_acceleration_structures,
                                                                 uint32_t count)
    {
        _as_infos.emplace_back(acceleration_structure_count, p_acceleration_structures);

        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_index]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
        write.setDstArrayElement(0);
        write.setPNext(&_as_infos.back());

        _writes.push_back(write);

        return *this;
    }
    
    Descriptor::Write& Descriptor::Write::uniform_buffer(uint32_t binding,
                                                         const vk::Buffer& buffer,
                                                         size_t offset,
                                                         size_t range,
                                                         uint32_t count)
    {
        auto info = _buffer_infos.emplace_back(buffer, offset, range);

        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_index]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eUniformBuffer);
        write.setDstArrayElement(0);
        write.setPBufferInfo(&_buffer_infos.back());

        _writes.push_back(write);

        return *this;
    }


    Descriptor::Write&
    Descriptor::Write::uniform_buffer(uint32_t binding, const vk::DescriptorBufferInfo& buffer, uint32_t count)
    {
        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_index]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eUniformBuffer);
        write.setDstArrayElement(0);
        write.setPBufferInfo(&buffer);

        _writes.push_back(write);

        return *this;
    }

    Descriptor::Write& Descriptor::Write::uniform_buffer_dynamic(uint32_t binding,
                                                         const vk::Buffer& buffer,
                                                         size_t offset,
                                                         size_t range,
                                                         uint32_t count)
    {
        auto info = _buffer_infos.emplace_back(buffer, offset, range);

        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_index]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic);
        write.setDstArrayElement(0);
        write.setPBufferInfo(&info);

        _writes.push_back(write);

        return *this;
    }

    Descriptor::Write&
    Descriptor::Write::uniform_buffer_dynamic(uint32_t binding, const vk::DescriptorBufferInfo& buffer_info, uint32_t count)
    {
        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_index]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic);
        write.setDstArrayElement(0);
        write.setPBufferInfo(&buffer_info);

        _writes.push_back(write);

        return *this;
    }


    Descriptor::Write& Descriptor::Write::combined_image_sampler(uint32_t binding,
                                                                 const vk::Sampler& sampler,
                                                                 const vk::ImageView& image_view,
                                                                 vk::ImageLayout image_layout,
                                                                 uint32_t count)
    {
        _image_infos.emplace_back(sampler, image_view, image_layout);

        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_index]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
        write.setDstArrayElement(0);
        write.setPImageInfo(&_image_infos.back());

        _writes.push_back(write);

        return *this;
    }

    
    Descriptor::Write& Descriptor::Write::combined_image_sampler(uint32_t binding,
                                                                 const vk::DescriptorImageInfo& image_info,
                                                                 uint32_t count)
    {
        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_index]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
        write.setDstArrayElement(0);
        write.setPImageInfo(&image_info);

        _writes.push_back(write);

        return *this;
    }

    
    Descriptor::Write& Descriptor::Write::storage_image(uint32_t binding,
                                                        const vk::DescriptorImageInfo& image_info,
                                                        uint32_t count)
    {
        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_index]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eStorageImage);
        write.setDstArrayElement(0);
        write.setPImageInfo(&image_info);

        _writes.push_back(write);

        return *this;
    }

    
    void Descriptor::Write::commit()
    {
        if (!_writes.empty())
        {
            _context.device().updateDescriptorSets(_writes.size(), _writes.data(), 0, nullptr);
        }
    }

    // Builder

    vk::DescriptorSetLayoutBinding
    Descriptor::Builder::make_binding(vk::DescriptorType type, uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        vk::DescriptorSetLayoutBinding b;
        b.setBinding(binding);
        b.setDescriptorCount(count);
        b.setDescriptorType(type);
        b.setStageFlags(shader_stage);
        return b;
    }

    vk::DescriptorSetLayoutBinding
    Descriptor::Builder::make_binding(DescriptorType type, uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        auto vk_type = get_vk_descriptor_type(type);

        vk::DescriptorSetLayoutBinding b;
        b.setBinding(binding);
        b.setDescriptorCount(count);
        b.setDescriptorType(vk_type);
        b.setStageFlags(shader_stage);
        return b;
    }

    Descriptor::Builder&
    Descriptor::Builder::add(DescriptorType type, uint32_t binding, vk::ShaderStageFlags shader_stages, uint32_t count)
    {
        _bindings.push_back(make_binding(type, binding, shader_stages, count));
        return *this;
    }

    #pragma region Descriptor Builder specific types

    Descriptor::Builder&
    Descriptor::Builder::sampler(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(DescriptorType::eSampler, binding, shader_stage, count));
        return *this;
    }

    Descriptor::Builder&
    Descriptor::Builder::sampled_image(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(DescriptorType::eSampledImage, binding, shader_stage, count));
        return *this;
    }

    Descriptor::Builder&
    Descriptor::Builder::combined_image_sampler(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(DescriptorType::eCombinedImageSampler, binding, shader_stage, count));
        return *this;
    }

    Descriptor::Builder&
    Descriptor::Builder::storage_buffer(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(DescriptorType::eStorageBuffer, binding, shader_stage, count));
        return *this;
    }

    Descriptor::Builder&
    Descriptor::Builder::storage_image(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(DescriptorType::eStorageImage, binding, shader_stage, count));
        return *this;
    }

    Descriptor::Builder&
    Descriptor::Builder::uniform_buffer(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(DescriptorType::eUniformBuffer, binding, shader_stage, count));
        return *this;
    }

    Descriptor::Builder&
    Descriptor::Builder::uniform_buffer_dynamic(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(DescriptorType::eUniformBufferDynamic, binding, shader_stage, count));
        return *this;
    }


    Descriptor::Builder&
    Descriptor::Builder::acceleration_structure(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(DescriptorType::eAccelerationStructure, binding, shader_stage, count));
        return *this;
    }

    #pragma endregion

    std::shared_ptr<Descriptor> Descriptor::Builder::create(uint32_t set_count, const sdvk::Context& context)
    {
        return Descriptor::Builder::create(set_count, context, "");
    }

    std::shared_ptr<Descriptor> Descriptor::Builder::create(uint32_t set_count, const sdvk::Context& context, const std::string& debug_name)
    {
        return std::make_shared<Descriptor>(set_count, _bindings, context, debug_name);
    }
}