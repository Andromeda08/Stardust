#pragma once

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
            static std::unique_ptr<ObjectsResource> create_from_scene(const std::shared_ptr<Scene>& resource)
            {
                auto result = std::make_unique<ObjectsResource>();
                result->m_resource = resource;
                return result;
            }
        };

        bool validate(std::shared_ptr<Output> const& incoming) override
        {
            auto res = std::dynamic_pointer_cast<Scene>(incoming);

            return res == nullptr;
        }

        const std::vector<sd::Object>& get_objects() const { return m_resource->objects(); }

        const std::array<float, 4>& get_color() override { return m_ui_color; }

        std::shared_ptr<Scene> m_resource;

    private:
        const std::array<float, 4> m_ui_color { 180.f, 190.f, 254.f, 255.f };
    };
}