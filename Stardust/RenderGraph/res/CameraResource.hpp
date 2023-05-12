#pragma once

#include <Scene/Camera.hpp>
#include <RenderGraph/Input.hpp>
#include <RenderGraph/Output.hpp>

namespace sd::rg
{
    struct CameraResource : public Input, public Output
    {
        struct Builder
        {
            Builder& with_name(std::string&& name)
            {
                _name = name;
                return *this;
            }

            std::unique_ptr<CameraResource> create_from_camera(const std::shared_ptr<sd::Camera>& resource)
            {
                auto result = std::make_unique<CameraResource>(_name);
                result->m_resource = resource;
                return result;
            }

        private:
            std::string _name {};
        };

        explicit CameraResource(std::string name): m_ui_name(std::move(name)) {}

        bool validate(std::shared_ptr<Output> const& incoming) override
        {
            auto res = std::dynamic_pointer_cast<sd::Camera>(incoming);

            return res == nullptr;
        }

        const std::array<int32_t, 4>& get_color() override { return m_ui_color; }

        const std::string& get_name() override { return m_ui_name; }

        std::shared_ptr<sd::Camera> m_resource;

    private:
        const std::array<int32_t, 4> m_ui_color { 239, 159, 118, 255 };
        const std::string m_ui_name {};
    };
}