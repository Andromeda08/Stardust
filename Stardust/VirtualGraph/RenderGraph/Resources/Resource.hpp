#pragma once

#include <memory>
#include <Nebula/Image.hpp>

namespace Nebula::RenderGraph
{
    class Resource
    {
    public:
        virtual ~Resource() = default;
    };

    class ImageResource : public Resource {
    public:
        ImageResource(const std::shared_ptr<Nebula::Image>& image)
        : Resource()
        , resource(image)
        {}

        std::shared_ptr<Nebula::Image> resource;
    };

    class DepthImageResource : public Resource {
    public:
        DepthImageResource(const std::shared_ptr<Nebula::Image>& depth_image)
        : Resource()
        , resource(depth_image)
        {}

        std::shared_ptr<Nebula::Image> resource;
    };

    class CameraResource : public Resource {};

    class TlasResource : public Resource {};

    class ObjectsResource : public Resource {};
}