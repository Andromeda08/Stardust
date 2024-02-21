#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Context.hpp>

namespace Nebula
{
    enum class DescriptorType
    {
        eCombinedImageSampler,
        eStorageBuffer,
        eStorageImage,
        eSampledImage,
        eSampler,
        eUniformBuffer,
        eUniformBufferDynamic,
        eAccelerationStructure,
    };

    static vk::DescriptorType get_vk_descriptor_type(DescriptorType descriptor_type);

    class Descriptor
    {
    public:
        struct Builder;
        struct Write;

        Descriptor(const Descriptor&) = delete;
        Descriptor& operator=(const Descriptor&)= delete;

        Descriptor(uint32_t set_count,
                   const std::vector<vk::DescriptorSetLayoutBinding>& bindings,
                   const sdvk::Context& context,
                   const std::string& debug_name);

        Write begin_write(uint32_t set_index);

        const vk::DescriptorSet& set(uint32_t index) const;

        const vk::DescriptorSetLayout& layout() const;

        const vk::DescriptorSet& operator[](uint32_t index) const;

        uint32_t set_count() const;

    private:
        void _create_layout();

        void _create_pool();

        void _create_descriptors();

    private:
        std::vector<vk::DescriptorSet> m_descriptors;
        std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
        vk::DescriptorSetLayout m_layout;
        vk::DescriptorPool m_pool;

        const std::string m_name;
        const uint32_t m_set_count;

        const sdvk::Context& m_context;
    };

    struct Descriptor::Builder
    {
        Builder& add(DescriptorType type, uint32_t binding, vk::ShaderStageFlags shader_stages, uint32_t count = 1);

        Builder& sampler(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

        Builder& sampled_image(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

        Builder& combined_image_sampler(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

        Builder& storage_buffer(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

        Builder& storage_image(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

        Builder& uniform_buffer(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

        Builder& uniform_buffer_dynamic(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

        Builder& acceleration_structure(uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count = 1);

        std::shared_ptr<Descriptor> create(uint32_t set_count, const sdvk::Context& context);

        std::shared_ptr<Descriptor> create(uint32_t set_count, const sdvk::Context& context, const std::string& debug_name);

    private:
        static vk::DescriptorSetLayoutBinding make_binding(vk::DescriptorType type, uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count);

        static vk::DescriptorSetLayoutBinding make_binding(DescriptorType type, uint32_t binding, vk::ShaderStageFlags shader_stage, uint32_t count);

        std::vector<vk::DescriptorSetLayoutBinding> _bindings;
        std::string _name;
    };

    struct Descriptor::Write
    {
        Write(const Descriptor& descriptor, uint32_t set_index, const sdvk::Context& context);

        Write& acceleration_structure(uint32_t binding, uint32_t acceleration_structure_count,
                                      const vk::AccelerationStructureKHR* p_acceleration_structures,
                                      uint32_t count = 1);


        Write& uniform_buffer(uint32_t binding, const vk::Buffer& buffer, vk::DeviceSize offset, vk::DeviceSize range,
                              uint32_t count = 1);

        Write& uniform_buffer(uint32_t binding, const vk::DescriptorBufferInfo& buffer, uint32_t count = 1);

        Write& uniform_buffer_dynamic(uint32_t binding, const vk::Buffer& buffer, size_t offset, size_t range,
                              uint32_t count = 1);


        Write& uniform_buffer_dynamic(uint32_t binding, const vk::DescriptorBufferInfo& buffer, uint32_t count = 1);

        Write& combined_image_sampler(uint32_t binding, const vk::Sampler& sampler, const vk::ImageView& image_view,
                                      vk::ImageLayout image_layout, uint32_t count = 1);

        Write& combined_image_sampler(uint32_t binding, const vk::DescriptorImageInfo& image_info, uint32_t count = 1);

        Write& storage_image(uint32_t binding, const vk::DescriptorImageInfo& image_info, uint32_t count = 1);

        Write& storage_buffer(uint32_t binding, const vk::Buffer& buffer, size_t offset, size_t range, uint32_t count = 1);

        void commit();

    private:
        std::vector<vk::WriteDescriptorSet> _writes;
        std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> _as_infos;
        std::vector<vk::DescriptorBufferInfo> _buffer_infos;
        std::vector<vk::DescriptorImageInfo> _image_infos;

        const uint32_t _set_index {0};
        const Descriptor& _descriptor;
        const sdvk::Context& _context;
    };
}