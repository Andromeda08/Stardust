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
            static std::unique_ptr<AccelerationStructureResource> create_from_scene(const std::shared_ptr<Scene>& resource)
            {
                auto result = std::make_unique<AccelerationStructureResource>();
                result->m_resource = resource->tlas();
                return result;
            }
        };

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

        const std::array<float, 4>& get_color() override { return m_ui_color; }

        std::shared_ptr<sdvk::Tlas> m_resource;

    private:
        const std::array<float, 4> m_ui_color { 180.f, 190.f, 254.f, 255.f };
    };
}