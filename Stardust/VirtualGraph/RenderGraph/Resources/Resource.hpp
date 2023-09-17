#pragma once

#include <memory>
#include <Nebula/Image.hpp>
#include <Scene/Camera.hpp>
#include <Scene/Object.hpp>
#include <Vulkan/Raytracing/Tlas.hpp>

namespace Nebula::RenderGraph
{
    struct Resource
    {
        virtual ~Resource() = default;
    };

    struct CameraResource : public Resource
    {
        explicit CameraResource(const std::shared_ptr<sd::Camera>& camera)
        : Resource(), m_camera(camera) {}

        std::shared_ptr<sd::Camera> m_camera;
    };

    struct DepthImageResource : public Resource
    {
        explicit DepthImageResource(const std::shared_ptr<Nebula::Image>& depth_image)
        : Resource(), resource(depth_image) {}

        std::shared_ptr<Nebula::Image> resource;
    };

    struct ImageResource : public Resource
    {
        explicit ImageResource(const std::shared_ptr<Nebula::Image>& image)
        : Resource(), resource(image) {}

        std::shared_ptr<Nebula::Image> resource;
    };

    struct ImageArrayResource : public Resource
    {
        explicit ImageArrayResource(const std::vector<std::shared_ptr<Nebula::Image>>& images)
        : Resource(), m_images(images) {}

        std::vector<std::shared_ptr<Nebula::Image>> m_images;
    };

    struct ObjectsResource : public Resource
    {
        explicit ObjectsResource(const std::vector<sd::Object>& objects)
        : Resource(), m_objects(objects) {}

        const std::vector<sd::Object>& m_objects;
    };

    struct TlasResource : public Resource
    {
        explicit TlasResource(const std::shared_ptr<sdvk::Tlas>& tlas)
        : Resource(), m_tlas(tlas) {}

        std::shared_ptr<sdvk::Tlas> m_tlas;
    };
}