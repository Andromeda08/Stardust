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
#include <VirtualGraph/RenderGraph/Nodes/BlurNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/PrePass.hpp>
#include <VirtualGraph/RenderGraph/Nodes/LightingPass.hpp>
#include <VirtualGraph/RenderGraph/Nodes/PresentNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/RayTracingNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/SceneProviderNode.hpp>

namespace Nebula::RenderGraph::Editor
{
    // Zinc
    class Node: public Nebula::Graph::Vertex
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

        struct Factory
        {
            static Node* create(NodeType type);
        };

        Node(const std::string& name, const glm::ivec4& color, const glm::ivec4& hover, NodeType type = NodeType::eUnknown);

        virtual void render();

        NodeType type() const;

        ResourceDescription& get_resource(int32_t id);

        ResourceDescription& get_resource(const std::string& name);

        const std::vector<ResourceDescription>& resources();

    protected:
        std::vector<ResourceDescription> m_resource_descriptions;

        virtual void render_options() {}

    private:
        glm::ivec4 m_color { 82, 82, 91, 255 };  // zinc-600
        glm::ivec4 m_hover { 161, 161, 170, 255 };  // zinc-400
        NodeType   m_type = NodeType::eUnknown;
    };

    class AmbientOcclusionNode: public Node
    {
    public:
        AmbientOcclusionNode();
    };

    class AntiAliasingNode: public Node
    {
    public:
        AntiAliasingNode();
    };

    class BlurNode: public Node
    {
    public:
        BlurNode();
    };

    class PrePassNode: public Node {
    public:
        PrePassNode();
    };

    class LightingPassNode : public Node {
    public:
        LightingPassNode();

        LightingPassOptions params;

    protected:
        void render_options() override;
    };

    class PresentNode : public Node
    {
    public:
        PresentNode();

        PresentNodeOptions params;

    protected:
        void render_options() override;
    };

    class SceneProviderNode : public Node
    {
    public:
        SceneProviderNode();
    };

    class RayTracingNode : public Node
    {
    public:
        RayTracingNode();

        RayTracingNodeOptions params;

    protected:
        void render_options() override;
    };
}