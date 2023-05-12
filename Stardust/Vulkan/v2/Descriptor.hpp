#pragma once

#include <array>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Context.hpp>

namespace sdvk
{
    /**
     * A class for creating and writing descriptors.
     * @tparam N number of descriptor sets.
     */
    template <unsigned int N>
    class Descriptor2
    {
    public:
        Descriptor2(const Descriptor2&) = delete;
        Descriptor2& operator=(const Descriptor2&) = delete;

        struct Builder
        {
            Builder& sampler(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

            Builder& sampled_image(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

            Builder& combined_image_sampler(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

            Builder& storage_buffer(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

            Builder& storage_image(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

            Builder& uniform_buffer(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

            Builder& acceleration_structure(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

            Builder& with_name(const std::string& name);

            std::unique_ptr<Descriptor2> create(const Context& context);

        private:
            static vk::DescriptorSetLayoutBinding make_binding(uint32_t binding, uint32_t count, vk::ShaderStageFlags shader_stage, vk::DescriptorType type);

            std::vector<vk::DescriptorSetLayoutBinding> _bindings;
            vk::DescriptorSetLayout _layout;
            std::string _name;
        };

        struct Write
        {
            Write(uint32_t set_id, const Descriptor2<N>& descriptor, const Context& context);

            Write& acceleration_structure(uint32_t binding, uint32_t acceleration_structure_count,
                                          const vk::AccelerationStructureKHR* p_acceleration_structures,
                                          uint32_t count = 1);


            Write& uniform_buffer(uint32_t binding, const vk::Buffer& buffer, uint32_t offset, uint32_t range,
                                  uint32_t count = 1);

            Write& combined_image_sampler(uint32_t binding, const vk::Sampler& sampler, const vk::ImageView& image_view,
                                          vk::ImageLayout image_layout, uint32_t count = 1);

            Write& combined_image_sampler(uint32_t binding, const vk::DescriptorImageInfo& image_info, uint32_t count = 1);

            void commit();

        private:
            std::vector<vk::WriteDescriptorSet> _writes;
            std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> _as_infos;
            std::vector<vk::DescriptorBufferInfo> _buffer_infos;
            std::vector<vk::DescriptorImageInfo> _image_infos;

            const Context&        _context;
            const Descriptor2<N>& _descriptor;
            const uint32_t        _set_id {0};
        };

        Descriptor2(const std::vector<vk::DescriptorSetLayoutBinding>& bindings,
                    const vk::DescriptorSetLayout& layout,
                    const Context& context);

        const vk::DescriptorSet& set(uint32_t id) const;

        const vk::DescriptorSetLayout& layout() const;

        const vk::DescriptorSet& operator[](uint32_t id) const;

        Write begin_write(uint32_t set_id);

    private:
        std::array<vk::DescriptorSet, N>            m_descriptor_sets;
        std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
        vk::DescriptorSetLayout                     m_layout;
        vk::DescriptorPool                          m_pool;
        const Context&                              m_context;
    };
}

#include "Descriptor.tpp"
