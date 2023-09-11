#pragma once

#include <format>
#include <stdexcept>
#include <string>
#include <vector>
#include <glm/vec4.hpp>
#include <imgui.h>
#include <uuid.h>
#include <Math/Graph/Vertex.hpp>
#include <VirtualGraph/ResourceDescription.hpp>
#include <VirtualGraph/Common/NodeType.hpp>
#include <VirtualGraph/RenderGraph/Nodes/RenderNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/SceneProviderNode.hpp>

namespace Nebula::Editor
{
    class Node : public Nebula::Graph::Vertex
    {
    private:
        glm::ivec4 m_color { 128, 128, 128, 255 };
        glm::ivec4 m_hover { 200, 200, 200, 255 };
        NodeType m_type = NodeType::eUnknown;

    protected:
        std::vector<ResourceDescription> m_resource_descriptions;

        virtual void render_options() {}

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

        const NodeType type() const { return m_type; }

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

        const std::vector<ResourceDescription>& resources() { return m_resource_descriptions; }
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

    class RenderNode : public Node
    {
    private:
        struct RenderGraph::RenderNodeOptions m_options;

    protected:
        void render_options() override
        {
            ImGui::Checkbox("Shadows", &m_options.with_shadows);
        }

    public:
        RenderNode()
        : Node("Render Node",
               { 114, 135, 253, 255 },
               { 180, 190, 254, 255 },
               NodeType::eRender)
        {
            const auto& specs = RenderGraph::RenderNode::s_resource_specs;
            for (const auto& spec : specs)
            {
                m_resource_descriptions.emplace_back(spec.name, spec.role, spec.type);
                m_resource_descriptions.back().spec = spec;
            }
        }
    };

    class AmbientOcclusionNode : public Node
    {
    private:
        struct AmbientOcclusionNodeOptions
        {
            bool rtao = true;
        } m_options;

    protected:
        void render_options() override
        {
            ImGui::Checkbox("Ray Traced", &m_options.rtao);
        }

    public:
        AmbientOcclusionNode()
        : Node("Ambient Occlusion",
               { 254, 100, 11, 255 },
               { 250, 179, 135, 255 },
               NodeType::eAmbientOcclusion)
        {
            m_resource_descriptions.emplace_back("G-Buffer", ResourceRole::eInput, ResourceType::eImage);
            m_resource_descriptions.emplace_back("Camera", ResourceRole::eInput, ResourceType::eCamera);
            m_resource_descriptions.emplace_back("TLAS", ResourceRole::eInput, ResourceType::eTlas);

            m_resource_descriptions.emplace_back("AO Buffer", ResourceRole::eOutput, ResourceType::eImage);
        }
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

    class CombineNode : public Node
    {
    public:
        CombineNode()
        : Node("Image Combine",
               { 32, 159, 181, 255 },
               { 116, 199, 236, 255 },
               NodeType::eCombine)
        {
            m_resource_descriptions.emplace_back("Image A", ResourceRole::eInput, ResourceType::eImage);
            m_resource_descriptions.emplace_back("Image B", ResourceRole::eInput, ResourceType::eImage);
            m_resource_descriptions.emplace_back("Result", ResourceRole::eOutput, ResourceType::eImage);
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
            m_resource_descriptions.emplace_back("Image", ResourceRole::eInput, ResourceType::eImage);
        }
    };

    class VirtualNodeFactory
    {
    public:
        Node* create(NodeType type)
        {
            switch (type)
            {
                case NodeType::eAmbientOcclusion:
                    return new AmbientOcclusionNode();
                case NodeType::eCombine:
                    return new CombineNode();
                case NodeType::eDenoise:
                    return new DenoiseNode();
                case NodeType::ePresent:
                    return new PresentNode();
                case NodeType::eRender:
                    return new RenderNode();
                case NodeType::eSceneProvider:
                    return new SceneProviderNode();
                default:
                    throw std::runtime_error("Invalid or not implemented node type.");
            }
        }
    };
}