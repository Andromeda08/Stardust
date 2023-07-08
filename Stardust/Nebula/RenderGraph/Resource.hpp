#pragma once

#include <memory>
#include <Nebula/Image.hpp>

namespace Nebula::RenderGraph
{
    class Resource
    {
    public:
        virtual bool connect(const Resource& resource) = 0;
        virtual bool validate(const Resource& resource) = 0;
    };

    class ImageResource : public Resource
    {
    public:
        const Image& image() const { return *m_image; }

    private:
        std::shared_ptr<Image> m_image;
    };
}