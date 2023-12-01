#pragma once

#include <string>
#include <vector>
#include <glm/vec4.hpp>
#include <uuid.h>
#include <Math/Graph/Vertex.hpp>
#include <VirtualGraph/Editor/ResourceDescription.hpp>
#include <VirtualGraph/Common/NodeType.hpp>
#include <VirtualGraph/RenderGraph/Nodes/LightingPass.hpp>
#include <VirtualGraph/RenderGraph/Nodes/PresentNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/RayTracingNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/SceneProviderNode.hpp>

#define DEF_BASIC_EDITOR_NODE(CN)  \
namespace Editor {                 \
    class CN final : public Node { \
    public:                        \
        CN();                      \
    };                             \
}

#define INT_DEF_BASIC_EDITOR_NODE(CN)   \
class CN final : public Node {          \
public:                                 \
    CN();                               \
};

namespace Nebula::RenderGraph::Editor
{
    class Node : public Graph::Vertex
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

            Node* create() const
            {
                const auto node = new Node(_name, _color, _hover);
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

        template <typename T>
        T& as()
        {
            static_assert(std::is_base_of_v<Node, T>, "Template parameter T must be a valid Node type");
            return dynamic_cast<T&>(*this);
        }

        NodeType type() const;

        ResourceDescription& get_resource(int32_t id);

        ResourceDescription& get_resource(const std::string& name);

        const std::vector<ResourceDescription>& resources();

    protected:
        std::vector<ResourceDescription> m_resource_descriptions;

        virtual void render_options() {}

    private:
        glm::ivec4 m_color { 82, 82, 91, 255 };
        glm::ivec4 m_hover { 161, 161, 170, 255 };
        NodeType   m_type = NodeType::eUnknown;
    };

    INT_DEF_BASIC_EDITOR_NODE(AmbientOcclusionNode);
    INT_DEF_BASIC_EDITOR_NODE(AntiAliasingNode);
    INT_DEF_BASIC_EDITOR_NODE(BlurNode);
    INT_DEF_BASIC_EDITOR_NODE(GBufferPass);
    INT_DEF_BASIC_EDITOR_NODE(SceneProviderNode);

    class LightingPassNode final : public Node {
    public:
        LightingPassNode();

        LightingPassOptions params;

    protected:
        void render_options() override;
    };

    class PresentNode final : public Node
    {
    public:
        PresentNode();

        PresentNodeOptions params;

    protected:
        void render_options() override;
    };

    class RayTracingNode final : public Node
    {
    public:
        RayTracingNode();

        RayTracingNodeOptions params;

    protected:
        void render_options() override;
    };
}