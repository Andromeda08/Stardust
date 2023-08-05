#include "Node.hpp"

#include <imgui.h>
#include <imnodes.h>

namespace Nebula::Editor
{
    Node::Node(const std::string& name, const glm::ivec4& color, const glm::ivec4& hover, NodeType type)
    : Nebula::Graph::Vertex(name)
    , m_color(color), m_hover(hover), m_type(type)
    {}

    void Node::render()
    {
        ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(m_color.r, m_color.g, m_color.b, 255));
        ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(m_hover.r, m_hover.g, m_hover.b, 255));
        {
            ImNodes::BeginNode(id());
            {
                ImNodes::BeginNodeTitleBar();
                {
                    ImGui::TextUnformatted(name().c_str());
                }
                ImNodes::EndNodeTitleBar();

                render_options();

                for (const auto& resource : m_resource_descriptions)
                {
                    auto id = resource.id;
                    auto pin_color = resource.pin_color();
                    ImNodes::PushColorStyle(ImNodesCol_Pin, IM_COL32(pin_color.r, pin_color.g, pin_color.b, 255));
                    switch (resource.role)
                    {
                        case ResourceRole::eInput:
                            ImNodes::BeginInputAttribute(id, ImNodesPinShape_CircleFilled);
                            break;
                        case ResourceRole::eOutput:
                            ImNodes::BeginOutputAttribute(id, ImNodesPinShape_CircleFilled);
                            break;
                        case ResourceRole::eUnknown:
                            continue;
                    }
                    {
                        ImGui::Text(resource.name.c_str());
                    }
                    switch (resource.role)
                    {
                        case ResourceRole::eInput:
                            ImNodes::EndInputAttribute();
                            break;
                        case ResourceRole::eOutput:
                            ImNodes::EndOutputAttribute();
                            break;
                        case ResourceRole::eUnknown:
                            break;
                    }
                    ImNodes::PopColorStyle();
                }
            }
            ImNodes::EndNode();
        }
        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();
    }
}