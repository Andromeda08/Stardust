#pragma once

#include <utility>
#include <vector>
#include <Scene/Camera.hpp>
#include <RenderGraph/Input.hpp>
#include <RenderGraph/Output.hpp>
#include <Scene/Object.hpp>
#include <Scene/Scene.hpp>

namespace sd::rg
{
    struct ObjectsResource : public Input, public Output
    {
        struct Builder
        {
            Builder& with_name(std::string&& name)
            {
                _name = name;
                return *this;
            }

            std::unique_ptr<ObjectsResource> create_from_scene(const std::shared_ptr<sd::Scene>& resource)
            {
                auto result = std::make_unique<ObjectsResource>(_name);
                result->m_resource = resource;
                return result;
            }

        private:
            std::string _name {};
        };

        explicit ObjectsResource(std::string name): m_ui_name(std::move(name)) {}

        bool validate(const Output& output) const override
        {
            try {
                auto res = dynamic_cast<const ObjectsResource&>(output);

                return true;
            }
            catch (const std::bad_cast& _ignored) {
                return false;
            }
        }

        void link_output(Output& input) override
        {
            auto res = dynamic_cast<ObjectsResource&>(input);
            m_resource = std::shared_ptr<sd::Scene>(res.m_resource);
        }

        const std::vector<sd::Object>& get_objects() const { return m_resource->objects(); }

        const std::array<int32_t, 4>& get_color() override { return m_ui_color; }

        const std::string& get_name() override { return m_ui_name; }

        std::shared_ptr<sd::Scene> m_resource;

    private:
        const std::array<int32_t, 4> m_ui_color { 30, 102, 245, 255 };
        const std::string m_ui_name {};
    };
}