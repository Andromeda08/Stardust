#pragma once

#include <memory>
#include <string>
#include <Nebula/Image.hpp>
#include <Scene/Camera.hpp>
#include <Scene/Object.hpp>
#include <Scene/Scene.hpp>
#include <Vulkan/Raytracing/Tlas.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceType.hpp>

#define PTR_RESOURCE_BODY(T, ResTypeEnum, ResT, GetName)                                                                                                                                                                                           \
public:                                                                                              \
    explicit T(const std::shared_ptr<ResT>& p_##GetName, const std::string& name = #T)               \
    : Resource(name, ResTypeEnum), m_resource(p_##GetName) {}                                        \
    [[nodiscard]] bool is_valid() override { return m_resource != nullptr; }                         \
    [[nodiscard]] const std::shared_ptr<ResT>& get_##GetName() const noexcept { return m_resource; } \
private:                                                                                             \
    std::shared_ptr<ResT> m_resource;

/**
 * This file contains all resource type definitions.
 * All resource types inherit the "Resource" class.
 */
namespace Nebula::RenderGraph
{
#pragma region Resource types
    using Buffer_t = sdvk::Buffer;
    using Buffer_ptr = std::shared_ptr<Buffer_t>;

    using Camera_t = sd::Camera;
    using Camera_ptr = std::shared_ptr<Camera_t>;

    using Image_t = Image;
    using Image_ptr = std::shared_ptr<Image_t>;

    using Object_t = sd::Object;
    using ObjectArray_t = std::vector<Object_t>;

    using Scene_t = sd::Scene;
    using Scene_ptr = std::shared_ptr<Scene_t>;

    using Tlas_t = sdvk::Tlas;
    using Tlas_ptr = std::shared_ptr<Tlas_t>;
#pragma endregion

    class Resource
    {
    public:
        Resource(const std::string& name, const ResourceType type): m_name(name), m_type(type) {}

        template <typename T>
        T& as()
        {
            static_assert(std::is_base_of_v<Resource, T>, "Template parameter T must be a valid Resource type");
            return dynamic_cast<T&>(*this);
        }

        virtual bool is_valid() = 0;

        virtual ~Resource() = default;

        [[nodiscard]] const std::string& name() const noexcept { return m_name; }

        [[nodiscard]] ResourceType type() const noexcept { return m_type; }

    private:
        std::string  m_name = "Unknown Resource";
        ResourceType m_type = ResourceType::eUnknown;
    };

    class BufferResource final : public Resource
    {
        PTR_RESOURCE_BODY(BufferResource, ResourceType::eBuffer, Buffer_t, buffer);
    };

    class CameraResource final : public Resource
    {
        PTR_RESOURCE_BODY(CameraResource, ResourceType::eCamera, Camera_t, camera);
    };

    class DepthImageResource final : public Resource
    {
        PTR_RESOURCE_BODY(DepthImageResource, ResourceType::eDepthImage, Image_t, depth_image);
    };

    class ImageResource final : public Resource
    {
        PTR_RESOURCE_BODY(ImageResource, ResourceType::eImage, Image_t, image);
    };

    class SceneResource final : public Resource
    {
        PTR_RESOURCE_BODY(SceneResource, ResourceType::eScene, Scene_t, scene);
    };

    class TlasResource final : public Resource
    {
        PTR_RESOURCE_BODY(TlasResource, ResourceType::eTlas, Tlas_t, tlas);
    };

    class ObjectsResource final : public Resource
    {
    public:
        explicit ObjectsResource(const ObjectArray_t& objects, const std::string& name = "Objects Resource")
        : Resource(name, ResourceType::eObjects)
        , m_objects(objects)
        {
        }

        bool is_valid() override
        {
            return !m_objects.empty();
        }

        const ObjectArray_t& get_objects() const
        {
            return m_objects;
        }

    private:
        const ObjectArray_t& m_objects;
    };
}
