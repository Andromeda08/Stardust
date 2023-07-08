#include "SceneNode.hpp"

#include <imgui.h>
#include <imnodes.h>
#include <RenderGraph/res/AccelerationStructureResource.hpp>
#include <RenderGraph/res/CameraResource.hpp>
#include <RenderGraph/res/ObjectsResource.hpp>

namespace sd::rg
{
    SceneNode::SceneNode(const std::shared_ptr<sd::Scene>& scene)
    : Node("Scene", {23, 146, 153, 255}, {129, 200, 190, 255})
    , m_scene(scene)
    {
        m_outputs.resize(3);
        m_outputs[0] = ObjectsResource::Builder()
                .with_name("Scene Objects")
                .create_from_scene(m_scene);
        m_outputs[1] = CameraResource::Builder()
                .with_name("Camera")
                .create_from_camera(m_scene->camera());
        m_outputs[2] = AccelerationStructureResource::Builder()
                .with_name("TLAS")
                .create_from_scene(m_scene);
    }

    void SceneNode::draw()
    {
        ImNodes::PushColorStyle(ImNodesCol_TitleBar, get_color().operator ImU32());
        ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, get_hover_color().operator ImU32());
        ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, get_hover_color().operator ImU32());

        ImNodes::BeginNode(m_id);
            ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted("Scene");
            ImNodes::EndNodeTitleBar();

            for (const auto& i : m_outputs)
            {
                ImNodes::PushColorStyle(ImNodesCol_Pin, i->imu32());
                ImNodes::BeginOutputAttribute(i->output_id());
                ImGui::Text(i->get_name().c_str());
                #ifdef SD_RG_DEBUG
                    ImGui::Text(std::to_string(i->output_id()).c_str());
                #endif
                ImNodes::EndOutputAttribute();
                ImNodes::PopColorStyle();
            }

        ImNodes::EndNode();

        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();
    }
}
