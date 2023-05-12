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

        bool validate(const Output& output) const override
        {
            try {
                auto res = dynamic_cast<const CameraResource&>(output);

                return true;
            }
            catch (const std::bad_cast& _ignored) {
                return false;
            }
        }

        void link_output(Output& input) override
        {
            auto res = dynamic_cast<CameraResource&>(input);
            m_resource = std::shared_ptr<sd::Camera>(res.m_resource);
        }

        const std::array<int32_t, 4>& get_color() override { return m_ui_color; }

        const std::string& get_name() override { return m_ui_name; }

        std::shared_ptr<sd::Camera> m_resource;

    private:
        const std::array<int32_t, 4> m_ui_color { 23, 146, 153, 255 };
        const std::string m_ui_name {};
    };
}