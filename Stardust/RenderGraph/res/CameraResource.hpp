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
            static std::unique_ptr<CameraResource> create_from_camera(const std::shared_ptr<sd::Camera>& resource)
            {
                auto result = std::make_unique<CameraResource>();
                result->m_resource = resource;
                return result;
            }
        };

        bool validate(std::shared_ptr<Output> const& incoming) override
        {
            auto res = std::dynamic_pointer_cast<sd::Camera>(incoming);

            return res == nullptr;
        }

        const std::array<float, 4>& get_color() override { return m_ui_color; }

        std::shared_ptr<sd::Camera> m_resource;

    private:
        const std::array<float, 4> m_ui_color { 180.f, 190.f, 254.f, 255.f };
    };
}