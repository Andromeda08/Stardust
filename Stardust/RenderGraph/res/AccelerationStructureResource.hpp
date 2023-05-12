#pragma once

#include <array>
#include <memory>
#include <string>
#include <RenderGraph/Input.hpp>
#include <RenderGraph/Output.hpp>
#include <RenderGraph/Scene.hpp>
#include <Vulkan/Raytracing/Tlas.hpp>

namespace sd::rg
{
    struct AccelerationStructureResource : public Input, public Output
    {
        struct Builder
        {
            Builder& with_name(std::string&& name)
            {
                _name = name;
                return *this;
            }

            std::unique_ptr<AccelerationStructureResource> create_from_scene(const std::shared_ptr<Scene>& resource)
            {
                auto result = std::make_unique<AccelerationStructureResource>(_name);
                result->m_resource = resource->tlas();
                return result;
            }

        private:
            std::string _name {};
        };

        explicit AccelerationStructureResource(std::string name): m_ui_name(std::move(name)) {}

        bool validate(std::shared_ptr<Output> const& incoming) override
        {
            auto res = std::dynamic_pointer_cast<AccelerationStructureResource>(incoming);

            // If dynamic cast fails the resource type isn't valid.
            if (res == nullptr)
            {
                return false;
            }

            return res->m_resource->tlas().objectType == vk::ObjectType::eAccelerationStructureKHR;
        }

        const std::array<int32_t, 4>& get_color() override { return m_ui_color; }

        const std::string& get_name() override { return m_ui_name; }

        std::shared_ptr<sdvk::Tlas> m_resource;

    private:
        const std::array<int32_t, 4> m_ui_color { 166, 209, 137, 255 };
        const std::string m_ui_name {};
    };
}