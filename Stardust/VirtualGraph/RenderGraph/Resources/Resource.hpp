#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-pass-by-value"
#pragma once

#include <memory>
#include <string>
#include <Nebula/Image.hpp>
#include <Scene/Camera.hpp>
#include <Scene/Object.hpp>
#include <Vulkan/Raytracing/Tlas.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceType.hpp>

/**
 * This file contains all resource type definitions.
 * All resource types inherit the "Resource" class.
 */
namespace Nebula::RenderGraph
{
    using Camera_t = sd::Camera;
    using Camera_ptr = std::shared_ptr<Camera_t>;

    using Image_t = Nebula::Image;
    using Image_ptr = std::shared_ptr<Image_t>;
    using ImageArray_t = std::vector<Image_ptr>;

    using Object_t = sd::Object;
    using ObjectArray_t = std::vector<Object_t>;

    using Tlas_t = sdvk::Tlas;
    using Tlas_ptr = std::shared_ptr<Tlas_t>;

    class Resource
    {
    public:
        Resource(const std::string& name, ResourceType type)
        : m_name(name)
        , m_type(type)
        {
        }

        /**
         * Allows for custom "validation" logic for specific Resource Types.
         * The most basic example: Is the resource a nullptr or not?
         */
        virtual bool is_valid() = 0;

        virtual ~Resource() = default;

        const std::string& name() const
        {
            return m_name;
        }

        ResourceType type() const
        {
            return m_type;
        }

    private:
        std::string  m_name = "Unknown Resource";
        ResourceType m_type = ResourceType::eUnknown;
    };

    class CameraResource : public Resource
    {
    public:
        explicit CameraResource(const Camera_ptr& camera, const std::string& name = "Camera Resource")
        : Resource(name, ResourceType::eCamera)
        , m_camera(camera)
        {
        }

        bool is_valid() override
        {
            return m_camera != nullptr;
        }

        const Camera_ptr& get_camera() const
        {
            return m_camera;
        }

    private:
         Camera_ptr m_camera;
    };

    class DepthImageResource : public Resource
    {
    public:
        explicit DepthImageResource(const Image_ptr& depth_image, const std::string& name = "Depth Image Resource")
        : Resource(name, ResourceType::eDepthImage)
        , m_depth_image(depth_image)
        {
        }

        bool is_valid() override
        {
            return m_depth_image != nullptr;
        }

        const Image_ptr& get_depth_image() const
        {
            return m_depth_image;
        }

    private:
        Image_ptr m_depth_image;
    };

    class ImageResource : public Resource
    {
        using Image_t = Nebula::Image;
        using Image_ptr = std::shared_ptr<Image_t>;
    public:
        explicit ImageResource(const Image_ptr& image, const std::string& name = "Image Resource")
        : Resource(name, ResourceType::eImage)
        , m_image(image)
        {
        }

        bool is_valid() override
        {
            return m_image != nullptr;
        }

        const Image_ptr& get_image() const
        {
            return m_image;
        }

    private:
        Image_ptr m_image;
    };

    class ImageArrayResource : public Resource
    {
    public:
        explicit ImageArrayResource(const ImageArray_t& images, const std::string& name = "Image Array Resource")
        : Resource(name, ResourceType::eImageArray)
        , m_images(images)
        {
        }

        bool is_valid() override
        {
            return m_images.empty();
        }

        const ImageArray_t& get_image_array() const
        {
            return m_images;
        }

    private:
        ImageArray_t m_images;
    };

    class ObjectsResource : public Resource
    {
    public:
        explicit ObjectsResource(const ObjectArray_t& objects, const std::string& name = "Objects Resource")
        : Resource(name, ResourceType::eObjects)
        , m_objects(objects)
        {
        }

        bool is_valid() override
        {
            return m_objects.empty();
        }

        const ObjectArray_t& get_objects() const
        {
            return m_objects;
        }

    private:
        const ObjectArray_t& m_objects;
    };

    class TlasResource : public Resource
    {
    public:
        explicit TlasResource(const Tlas_ptr& tlas, const std::string& name = "Tlas Resource")
        : Resource(name, ResourceType::eTlas)
        , m_tlas(tlas)
        {
        }

        bool is_valid() override
        {
            return m_tlas != nullptr;
        }

        const Tlas_ptr& get_tlas() const
        {
            return m_tlas;
        }

    private:
        Tlas_ptr m_tlas;
    };
}

#pragma clang diagnostic pop
