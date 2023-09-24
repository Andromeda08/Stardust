#pragma once

#include <format>
#include <stdexcept>
#include <string>
#include <vector>
#include <glm/vec4.hpp>
#include <imgui.h>
#include <uuid.h>
#include <Math/Graph/Vertex.hpp>
#include <VirtualGraph/Editor/ResourceDescription.hpp>
#include <VirtualGraph/Common/NodeType.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AmbientOcclusionNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AntiAliasingNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/DeferredRender.hpp>
#include <VirtualGraph/RenderGraph/Nodes/LightingPass.hpp>
#include <VirtualGraph/RenderGraph/Nodes/PresentNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/SceneProviderNode.hpp>

namespace Nebula::RenderGraph::Editor
{
    class Node : public Nebula::Graph::Vertex
    {
    public:
        struct Builder
        {
            Builder& with_colors(const glm::ivec4& color, const glm::ivec4& hover)
            {
                _color = color;
                _hover = hover;
                return *this;
            }

            Builder& with_name(const std::string& name)
            {
                _name = name;
                return *this;
            }

            Builder& with_resources(const std::vector<ResourceDescription>& resources)
            {
                _resources = resources;
                return *this;
            }

            Node* create()
            {
                auto node = new Node(_name, _color, _hover);
                node->m_resource_descriptions = _resources;
                return node;
            }

        private:
            glm::ivec4 _color { 128, 128, 128, 255 };
            glm::ivec4 _hover { 200, 200, 200, 255 };
            std::string _name = "Unknown Node";
            std::vector<ResourceDescription> _resources;
        };

        Node(const std::string& name, const glm::ivec4& color, const glm::ivec4& hover, NodeType type = NodeType::eUnknown);

        virtual void render();

        NodeType type() const
        {
            return m_type;
        }

        ResourceDescription& get_resource(int32_t id)
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

        const std::vector<ResourceDescription>& resources()
        {
            return m_resource_descriptions;
        }

    protected:
        std::vector<ResourceDescription> m_resource_descriptions;

        virtual void render_options() {}

    private:
        glm::ivec4 m_color { 128, 128, 128, 255 };
        glm::ivec4 m_hover { 200, 200, 200, 255 };
        NodeType   m_type = NodeType::eUnknown;
    };

    class AmbientOcclusionNode : public Node
    {
    public:
        AmbientOcclusionNode()
        : Node("Ambient Occlusion",
               { 254, 100, 11, 255 },
               { 250, 179, 135, 255 },
               NodeType::eAmbientOcclusion)
        {
            const auto& specs = RenderGraph::AmbientOcclusionNode::s_resource_specs;
            for (const auto& spec : specs)
            {
                m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
                m_resource_descriptions.back().spec = spec;
            }
        }
    };

    class AntiAliasingNode : public Node {
    public:
        AntiAliasingNode()
        : Node("Anti-Aliasing",
               { 254, 100, 11, 255 },
               { 250, 179, 135, 255 },
               NodeType::eAntiAliasing)
        {
            const auto& specs = RenderGraph::AntiAliasingNode::s_resource_specs;
            for (const auto& spec : specs)
            {
                m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
                m_resource_descriptions.back().spec = spec;
            }
        }
    };

    class DeferredPassNode : public Node {
    public:
        DeferredPassNode()
        : Node("Deferred Pass",
               { 230, 69, 83, 255 },
               { 235, 160, 172, 255 },
               NodeType::eDeferredRender)
        {
            const auto& specs = RenderGraph::DeferredRender::s_resource_specs;
            for (const auto& spec : specs)
            {
                m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
                m_resource_descriptions.back().spec = spec;
            }
        }
    };

    class LightingPassNode : public Node {
    public:
        LightingPassNode()
        : Node("Lighting Pass",
               { 23, 146, 153, 255 },
               { 148, 226, 213, 255 },
               NodeType::eLightingPass)
        {
            const auto& specs = RenderGraph::LightingPass::s_resource_specs;
            for (const auto& spec : specs)
            {
                m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
                m_resource_descriptions.back().spec = spec;
            }
        }

    protected:
        void render_options() override
        {
            ImGui::Checkbox("With Ambient Occlusion", &m_options.include_ao);
            ImGui::Checkbox("With Anti-Aliasing", &m_options.include_aa);
            ImGui::Checkbox("With Shadows", &m_options.with_shadows);
        }

    private:
        LightingPassOptions m_options;
    };

    class DenoiseNode : public Node
    {
    public:
        DenoiseNode()
            : Node("Denoiser",
                   { 23, 146, 153, 255 },
                   { 148, 226, 213, 255 },
                   NodeType::eDenoise)
        {
            m_resource_descriptions.emplace_back("Input Image", ResourceRole::eInput, ResourceType::eImage);
            m_resource_descriptions.emplace_back("De-noised Image", ResourceRole::eOutput, ResourceType::eImage);
        }
    };

    class PresentNode : public Node
    {
    public:
        PresentNode()
        : Node("Present",
               { 221, 120, 120, 255 },
               { 242, 205, 205, 255 },
               NodeType::ePresent)
        {

            const auto& specs = RenderGraph::PresentNode::s_resource_specs;
            for (const auto& spec : specs)
            {
                m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
                m_resource_descriptions.back().spec = spec;
            }
        }
    };

    class SceneProviderNode : public Node
    {
    public:
        SceneProviderNode()
            : Node("Scene Provider",
                   { 4, 165, 229, 255 },
                   { 137, 220, 235, 255 },
                   NodeType::eSceneProvider)
        {
            const auto& specs = RenderGraph::SceneProviderNode::s_resource_specs;
            for (const auto& spec : specs)
            {
                m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
                m_resource_descriptions.back().spec = spec;
            }
        }
    };

    class VirtualNodeFactory
    {
    public:
        static Node* create(NodeType type)
        {
            switch (type)
            {
                case NodeType::eAmbientOcclusion:
                    return new AmbientOcclusionNode();
                case NodeType::eAntiAliasing:
                    return new AntiAliasingNode();
                case NodeType::eDeferredRender:
                    return new DeferredPassNode();
                case NodeType::eDenoise:
                    return new DenoiseNode();
                case NodeType::eLightingPass:
                    return new LightingPassNode();
                case NodeType::ePresent:
                    return new PresentNode();
                case NodeType::eSceneProvider:
                    return new SceneProviderNode();
                default:
                    throw std::runtime_error("Invalid or not implemented node type.");
            }
        }
    };
}