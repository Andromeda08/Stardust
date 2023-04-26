#include "SceneNode.hpp"

#include <RenderGraph/res/CameraResource.hpp>
#include <RenderGraph/res/ObjectsResource.hpp>

namespace sd::rg
{
    SceneNode::SceneNode(const std::shared_ptr<Scene>& scene)
    : m_scene(scene)
    {
        m_outputs.resize(3);
        m_outputs[0] = CameraResource::Builder::create_from_camera(m_scene->camera());
        m_outputs[1] = ObjectsResource::Builder::create_from_scene(m_scene);
        m_outputs[2] = AccelerationStructureResource::Builder::create_from_scene(m_scene);
    }
}
