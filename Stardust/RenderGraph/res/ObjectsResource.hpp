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

            std::unique_ptr<ObjectsResource> create_from_scene(const std::shared_ptr<Scene>& resource)
            {
                auto result = std::make_unique<ObjectsResource>(_name);
                result->m_resource = resource;
                return result;
            }

        private:
            std::string _name {};
        };

        explicit ObjectsResource(std::string name): m_ui_name(std::move(name)) {}

        bool validate(std::shared_ptr<Output> const& incoming) override
        {
            auto res = std::dynamic_pointer_cast<Scene>(incoming);

            return res == nullptr;
        }

        const std::vector<sd::Object>& get_objects() const { return m_resource->objects(); }

        const std::array<int32_t, 4>& get_color() override { return m_ui_color; }

        const std::string& get_name() override { return m_ui_name; }

        std::shared_ptr<Scene> m_resource;

    private:
        const std::array<int32_t, 4> m_ui_color { 148, 226, 213, 255 };
        const std::string m_ui_name {};
    };
}