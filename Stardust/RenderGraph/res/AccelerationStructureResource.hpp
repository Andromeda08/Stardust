#pragma once

#include <array>
#include <memory>
#include <string>
#include <RenderGraph/Input.hpp>
#include <RenderGraph/Output.hpp>
#include <Scene/Scene.hpp>
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

            std::unique_ptr<AccelerationStructureResource> create_from_scene(const std::shared_ptr<sd::Scene>& resource)
            {
                auto result = std::make_unique<AccelerationStructureResource>(_name);
                result->m_resource = resource->acceleration_structure();
                return result;
            }

        private:
            std::string _name {};
        };

        explicit AccelerationStructureResource(std::string name): m_ui_name(std::move(name)) {}

        bool validate(const Output& output) const override
        {
            try {
                auto res = dynamic_cast<const AccelerationStructureResource&>(output);

                return res.m_resource->tlas().objectType == vk::ObjectType::eAccelerationStructureKHR;
            }
            catch (const std::bad_cast& _ignored) {
                return false;
            }
        }

        void link_output(Output& input) override
        {
            auto res = dynamic_cast<const AccelerationStructureResource&>(input);
            m_resource = std::shared_ptr<sdvk::Tlas>(res.m_resource);
        }

        const std::array<int32_t, 4>& get_color() override { return m_ui_color; }

        const std::string& get_name() override { return m_ui_name; }

        std::shared_ptr<sdvk::Tlas> m_resource;

    private:
        const std::array<int32_t, 4> m_ui_color { 64, 160, 43, 255 };
        const std::string m_ui_name {};
    };
}