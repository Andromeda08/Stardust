#include "Node.hpp"

#include <format>
#include <stdexcept>
#include <imgui.h>
#include <imnodes.h>
#include <VirtualGraph/RenderGraph/Nodes/AmbientOcclusionNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AntiAliasingNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/BlurNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/GBufferPass.hpp>
#include <VirtualGraph/RenderGraph/Nodes/MeshGBufferPass.hpp>

namespace Nebula::RenderGraph::Editor
{
    Node::Node(const std::string& name, const glm::ivec4& color, const glm::ivec4& hover, const NodeType type)
    : Graph::Vertex(name), m_color(color), m_hover(hover), m_type(type)
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
                    const auto id = resource.id;
                    const auto pin_color = resource.pin_color();
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

    NodeType Node::type() const
    {
        return m_type;
    }

    ResourceDescription& Node::get_resource(int32_t id)
    {
        for (auto& item : m_resource_descriptions)
        {
            if (id == item.id)
            {
                return item;
            }
        }

        throw std::runtime_error(std::format("No resource by the id {}", id));
    }

    ResourceDescription& Node::get_resource(const std::string& name)
    {
        for (auto& item : m_resource_descriptions)
        {
            if (name == item.name)
            {
                return item;
            }
        }

        throw std::runtime_error(std::format("No resource by the name {}", name));
    }

    const std::vector<ResourceDescription>& Node::resources()
    {
        return m_resource_descriptions;
    }

    #pragma region Node color constants
    /**
     * Node and resource colors are taken from the default Tailwind color palette
     * Hover -> 400, Color -> 600
     * https://tailwindcss.com/docs/customizing-colors
     */

    // Red
    constexpr glm::ivec4 ao_color = { 220, 38, 38, 255 };
    constexpr glm::ivec4 ao_hover = { 248, 113, 113, 255 };

    // Lime
    constexpr glm::ivec4 aa_color = { 101, 163, 13, 255 };
    constexpr glm::ivec4 aa_hover = { 163, 230, 53, 255 };

    // Cyan
    constexpr glm::ivec4 blur_color = { 8, 145, 178, 255 };
    constexpr glm::ivec4 blur_hover = { 34, 211, 238, 255 };

    // Indigo
    constexpr glm::ivec4 pre_pass_color = { 79, 70, 229, 255 };
    constexpr glm::ivec4 pre_pass_hover = { 129, 140, 248, 255 };

    // Amber
    constexpr glm::ivec4 lighting_pass_color = { 217, 119, 6, 255 };
    constexpr glm::ivec4 lighting_pass_hover = { 251, 191, 36, 255 };

    // Fuchsia
    constexpr glm::ivec4 present_color = { 192, 38, 211, 255 };
    constexpr glm::ivec4 present_hover = {232, 121, 249, 255 };

    // Blue
    constexpr glm::ivec4 scene_provider_color = {37, 99, 235, 255 };
    constexpr glm::ivec4 scene_provider_hover = {96, 165, 250, 255 };

    // Violet
    constexpr glm::ivec4 rt_color = { 124, 58, 237, 255 };
    constexpr glm::ivec4 rt_hover = { 167, 139, 250, 255 };

    #pragma endregion

    #pragma region Node types

    AmbientOcclusionNode::AmbientOcclusionNode()
    : Node("Ambient Occlusion", ao_color, ao_hover, NodeType::eAmbientOcclusion)
    {
        for (const auto& spec : RenderGraph::AmbientOcclusionNode::s_resource_specs)
        {
            m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
            m_resource_descriptions.back().spec = spec;
        }
    }

    AntiAliasingNode::AntiAliasingNode()
    : Node("Anti-Aliasing", aa_color, aa_hover, NodeType::eAntiAliasing)
    {
        for (const auto& spec : RenderGraph::AntiAliasingNode::s_resource_specs)
        {
            m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
            m_resource_descriptions.back().spec = spec;
        }
    }

    BlurNode::BlurNode()
    : Node("Gaussian Blur", blur_color, blur_hover, NodeType::eGaussianBlur)
    {
        for (const auto& spec : RenderGraph::BlurNode::s_resource_specs)
        {
            m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
            m_resource_descriptions.back().spec = spec;
        }
    }


    GBufferPass::GBufferPass()
    : Node("G-Buffer Pass", pre_pass_color, pre_pass_hover, NodeType::eGBufferPass)
    {
        for (const auto& spec : RenderGraph::GBufferPass::s_resource_specs)
        {
            m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
            m_resource_descriptions.back().spec = spec;
        }
    }

    LightingPassNode::LightingPassNode()
    : Node("Lighting Pass", lighting_pass_color, lighting_pass_hover, NodeType::eLightingPass)
    {
        for (const auto& spec : RenderGraph::LightingPass::s_resource_specs)
        {
            m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
            m_resource_descriptions.back().spec = spec;
        }
    }

    void LightingPassNode::render_options()
    {
        ImGui::Checkbox("Use Ambient Occlusion", &params.ambient_occlusion);
        ImGui::Checkbox("Raytraced Shadows", &params.enable_shadows);
    }

    PresentNode::PresentNode()
    : Node("Present", present_color, present_hover, NodeType::ePresent)
    {
        for (const auto& spec : RenderGraph::PresentNode::s_resource_specs)
        {
            m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
            m_resource_descriptions.back().spec = spec;
        }
    }

    void PresentNode::render_options()
    {
        ImGui::Checkbox("Flip Image", &params.flip_image);
    }

    SceneProviderNode::SceneProviderNode()
    : Node("Scene Provider", scene_provider_color, scene_provider_hover, NodeType::eSceneProvider)
    {
        for (const auto& spec : RenderGraph::SceneProviderNode::s_resource_specs)
        {
            m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
            m_resource_descriptions.back().spec = spec;
        }
    }

    RayTracingNode::RayTracingNode()
    : Node("Ray Tracing", rt_color, rt_hover, NodeType::eRayTracing)
    {
        for (const auto& spec : RenderGraph::RayTracingNode::s_resource_specs)
        {
            m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
            m_resource_descriptions.back().spec = spec;
        }
    }

    void RayTracingNode::render_options()
    {
        ImGui::PushItemWidth(128);
        {
            ImGui::SliderScalar("Ray Bounces", ImGuiDataType_S32, &params.reflection_count, &params.reflection_bounds.x,
                                &params.reflection_bounds.y, "%d");
        }
        ImGui::PopItemWidth();
    }

    #pragma endregion
}
