#include <Vulkan/Utils.hpp>

namespace sdvk
{
    #pragma region Descriptor Builder implementation
    template<unsigned int N>
    Descriptor2<N>::Builder&
    Descriptor2<N>::Builder::sampler(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(binding, count, shader_stage, vk::DescriptorType::eSampler));
        return *this;
    }

    template<unsigned int N>
    Descriptor2<N>::Builder&
    Descriptor2<N>::Builder::sampled_image(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(binding, count, shader_stage, vk::DescriptorType::eSampledImage));
        return *this;
    }

    template<unsigned int N>
    Descriptor2<N>::Builder&
    Descriptor2<N>::Builder::combined_image_sampler(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(binding, count, shader_stage, vk::DescriptorType::eCombinedImageSampler));
        return *this;
    }

    template<unsigned int N>
    Descriptor2<N>::Builder&
    Descriptor2<N>::Builder::storage_buffer(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(binding, count, shader_stage, vk::DescriptorType::eStorageBuffer));
        return *this;
    }

    template<unsigned int N>
    Descriptor2<N>::Builder&
    Descriptor2<N>::Builder::storage_image(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(binding, count, shader_stage, vk::DescriptorType::eStorageImage));
        return *this;
    }

    template<unsigned int N>
    Descriptor2<N>::Builder&
    Descriptor2<N>::Builder::uniform_buffer(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(binding, count, shader_stage, vk::DescriptorType::eUniformBuffer));
        return *this;
    }

    template<unsigned int N>
    Descriptor2<N>::Builder&
    Descriptor2<N>::Builder::acceleration_structure(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count)
    {
        _bindings.push_back(make_binding(binding, count, shader_stage, vk::DescriptorType::eAccelerationStructureKHR));
        return *this;
    }

    template<unsigned int N>
    Descriptor2<N>::Builder&
    Descriptor2<N>::Builder::with_name(const std::string& name)
    {
        _name = name;
        return *this;
    }

    template<unsigned int N>
    std::unique_ptr<Descriptor2<N>>
    Descriptor2<N>::Builder::create(const Context& context)
    {
        if (_bindings.empty())
        {
            throw std::runtime_error("No descriptor bindings.");
        }

        vk::DescriptorSetLayoutCreateInfo create_info;
        create_info.setBindingCount(_bindings.size());
        create_info.setPBindings(_bindings.data());

        vk::Result result = context.device().createDescriptorSetLayout(&create_info, nullptr, &_layout);

        if (!_layout)
        {
            throw std::runtime_error("Failed to create DescriptorSetLayout.");
        }

        auto r = std::make_unique<Descriptor2<N>>(_bindings, _layout, context);

        if (!_name.empty())
        {
            for (int32_t i = 0; i < N; i++)
            {
                std::string name = "Descriptor "  + _name + " #" + std::to_string(i);
                sdvk::util::name_vk_object(name, (uint64_t) static_cast<VkDescriptorSet>(r->set(i)), vk::ObjectType::eDescriptorSet, context.device());
            }
        }

        return r;

    }

    template<unsigned int N>
    vk::DescriptorSetLayoutBinding
    Descriptor2<N>::Builder::make_binding(uint32_t binding, uint32_t count, vk::ShaderStageFlags shader_stage, vk::DescriptorType type)
    {
        vk::DescriptorSetLayoutBinding b;
        b.setBinding(binding);
        b.setDescriptorCount(count);
        b.setDescriptorType(type);
        b.setStageFlags(shader_stage);
        return b;
    }
    #pragma endregion

    template<unsigned int N>
    Descriptor2<N>::Descriptor2(const std::vector<vk::DescriptorSetLayoutBinding>& bindings,
                                const vk::DescriptorSetLayout& layout,
                                const Context& context)
    : m_bindings(bindings)
    , m_context(context)
    , m_layout(layout)
    {
        std::vector<vk::DescriptorPoolSize> pool_sizes;
        for (const auto& b : m_bindings)
        {
            pool_sizes.emplace_back(b.descriptorType, b.descriptorCount);
        }

        vk::DescriptorPoolCreateInfo pool_create_info;
        pool_create_info.setMaxSets(N);
        pool_create_info.setPoolSizeCount(pool_sizes.size());
        pool_create_info.setPPoolSizes(pool_sizes.data());
        auto result = m_context.device().createDescriptorPool(&pool_create_info, nullptr, &m_pool);

        std::vector<vk::DescriptorSetLayout> layouts(N, m_layout);
        vk::DescriptorSetAllocateInfo allocate_info;
        allocate_info.setDescriptorPool(m_pool);
        allocate_info.setDescriptorSetCount(N);
        allocate_info.setPSetLayouts(layouts.data());

        result = m_context.device().allocateDescriptorSets(&allocate_info, m_descriptor_sets.data());
    }

    template<unsigned int N>
    const vk::DescriptorSet& Descriptor2<N>::set(uint32_t id) const
    {
        if (id > m_descriptor_sets.size())
        {
            throw std::out_of_range("Index" + std::to_string(id) + " out of range for descriptor set.");
        }

        return m_descriptor_sets[id];
    }

    template<unsigned int N>
    const vk::DescriptorSet& Descriptor2<N>::operator[](uint32_t id) const
    {
        return set(id);
    }

    template<unsigned int N>
    const vk::DescriptorSetLayout& Descriptor2<N>::layout() const
    {
        return m_layout;
    }

    template<unsigned int N>
    Descriptor2<N>::Write Descriptor2<N>::begin_write(uint32_t set_id)
    {
        return Descriptor2<N>::Write(set_id, *this, m_context);
    }

    #pragma region Descriptor Write implementation
    template<unsigned int N>
    sdvk::Descriptor2<N>::Write::Write(uint32_t set_id, const Descriptor2& descriptor, const Context& context)
    : _context(context), _descriptor(descriptor), _set_id(set_id) {}

    template<unsigned int N>
    Descriptor2<N>::Write&
    Descriptor2<N>::Write::acceleration_structure(uint32_t binding, uint32_t acceleration_structure_count,
                                                  const vk::AccelerationStructureKHR* p_acceleration_structures,
                                                  uint32_t count)
    {
        _as_infos.emplace_back(acceleration_structure_count, p_acceleration_structures);

        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_id]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
        write.setDstArrayElement(0);
        write.setPNext(&_as_infos.back());

        _writes.push_back(write);

        return *this;
    }

    template<unsigned int N>
    Descriptor2<N>::Write&
    Descriptor2<N>::Write::acceleration_structure(uint32_t binding, const vk::WriteDescriptorSetAccelerationStructureKHR& pNext)
    {
        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_id]);
        write.setDescriptorCount(1);
        write.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
        write.setDstArrayElement(0);
        write.setPNext(&pNext);

        _writes.push_back(write);
        return *this;
    }

    template<unsigned int N>
    Descriptor2<N>::Write&
    Descriptor2<N>::Write::uniform_buffer(uint32_t binding, const vk::Buffer& buffer, uint32_t offset, uint32_t range,
                                          uint32_t count)
    {
        _buffer_infos.emplace_back(buffer, offset, range);

        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_id]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eUniformBuffer);
        write.setDstArrayElement(0);
        write.setPBufferInfo(&_buffer_infos.back());

        _writes.push_back(write);

        return *this;
    }

    template<unsigned int N>
    Descriptor2<N>::Write&
    Descriptor2<N>::Write::combined_image_sampler(uint32_t binding, const vk::Sampler& sampler, const vk::ImageView& image_view,
                                                  vk::ImageLayout image_layout, uint32_t count)
    {
        _image_infos.emplace_back(sampler, image_view, image_layout);

        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_id]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
        write.setDstArrayElement(0);
        write.setPImageInfo(&_image_infos.back());

        _writes.push_back(write);

        return *this;
    }

    template<unsigned int N>
    Descriptor2<N>::Write&
    Descriptor2<N>::Write::combined_image_sampler(uint32_t binding, const vk::DescriptorImageInfo& image_info, uint32_t count)
    {
        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_id]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
        write.setDstArrayElement(0);
        write.setPImageInfo(&image_info);

        _writes.push_back(write);

        return *this;
    }

    template<unsigned int N>
    Descriptor2<N>::Write&
    Descriptor2<N>::Write::storage_image(uint32_t binding, const vk::DescriptorImageInfo& image_info, uint32_t count)
    {
        vk::WriteDescriptorSet write;
        write.setDstBinding(binding);
        write.setDstSet(_descriptor[_set_id]);
        write.setDescriptorCount(count);
        write.setDescriptorType(vk::DescriptorType::eStorageImage);
        write.setDstArrayElement(0);
        write.setPImageInfo(&image_info);

        _writes.push_back(write);

        return *this;
    }

    template<unsigned int N>
    void Descriptor2<N>::Write::commit()
    {
        if (!_writes.empty())
        {
            _context.device().updateDescriptorSets(_writes.size(), _writes.data(), 0, nullptr);
        }
    }
    #pragma endregion
}