#pragma once

#include <map>
#include <memory>
#include <vector>
#include <Utility.hpp>
#include <Scene/Scene.hpp>
#include <VirtualGraph/Compile/GraphCompileStrategy.hpp>
#include <VirtualGraph/Common/GraphContext.hpp>
#include <VirtualGraph/Editor/Edge.hpp>
#include <VirtualGraph/Editor/Node.hpp>

namespace Nebula::RenderGraph::Editor
{
    class GraphEditor
    {
    private:
        using id_t = int32_t;
        using node_ptr_t = std::shared_ptr<Node>;
        using Scene_t = sd::Scene;

        std::map<id_t, node_ptr_t> m_nodes;
        std::vector<Edge>          m_edges;

        std::vector<std::string>              m_messages;
        std::unique_ptr<VirtualNodeFactory>   m_factory;
        std::unique_ptr<Compiler::GraphCompileStrategy> m_compiler;

        // Flags for root and sink nodes
        bool m_has_scene_provider { false };
        bool m_has_presenter { false };

        RenderGraphContext& m_context;

    public:
        // TODO: Should replace Scene with a new representation
        static std::shared_ptr<Scene_t> s_selected_scene;

    public:
        explicit GraphEditor(RenderGraphContext& context);

        void render();

        static void set_scene(const std::shared_ptr<Scene_t>& scene)
        {
            s_selected_scene = std::shared_ptr<Scene_t>(scene);
        }

    private:
        void _handle_compile();

        bool _handle_connection();

        void _handle_link_delete();

        void _erase_edge(id_t edge_id);

        void _handle_add_node(NodeType type);
    };
}