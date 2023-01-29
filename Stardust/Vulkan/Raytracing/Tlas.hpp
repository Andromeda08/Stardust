#pragma once

#include <vulkan/vulkan.hpp>
#include <Scene/Object.hpp>
#include <Vulkan/Buffer.hpp>

namespace sdvk
{
    class Tlas
    {
    public:
        struct Builder
        {
            Builder() = default;

            Builder& with_name(std::string const& name)
            {
                _name = name;
                return *this;
            }

            std::unique_ptr<Tlas> create(std::vector<sd::Object> const& objects, CommandBuffers const& command_buffers, Context const& context)
            {
                auto result = std::make_unique<Tlas>(objects, command_buffers, context);

                if (context.is_debug())
                {
                    vk::DebugUtilsObjectNameInfoEXT name_info;
                    name_info.setObjectHandle((uint64_t) static_cast<VkAccelerationStructureKHR>(result->tlas()));
                    name_info.setObjectType(vk::ObjectType::eAccelerationStructureKHR);
                    name_info.setPObjectName(_name.c_str());
                    auto r = context.device().setDebugUtilsObjectNameEXT(&name_info);
                }

                return result;
            }

        private:
            std::string _name;
        };

        Tlas(std::vector<sd::Object> const& objects, CommandBuffers const& command_buffers, Context const& context);

        void rebuild(std::vector<sd::Object> const& objects);

        const vk::AccelerationStructureKHR& tlas() const { return m_tlas; }

    private:
        void create(std::vector<sd::Object> const& objects);

        void build_instance_data(std::vector<sd::Object> const& objects);

        void build_top_level_as();

    private:
        vk::AccelerationStructureKHR m_tlas;
        std::unique_ptr<Buffer>      m_buffer;
        std::unique_ptr<Buffer>      m_instance_data;
        uint32_t                     m_instance_count { 0 };

        const CommandBuffers& m_command_buffers;
        const Context& m_context;
    };
}