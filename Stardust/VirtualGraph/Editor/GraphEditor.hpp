#pragma once

#include <map>
#include <memory>
#include <vector>
#include <Scene/Scene.hpp>
#include <VirtualGraph/Compile/CompilerType.hpp>
#include <VirtualGraph/Compile/GraphCompileStrategy.hpp>
#include <VirtualGraph/Common/GraphContext.hpp>
#include <VirtualGraph/Editor/Edge.hpp>
#include <VirtualGraph/Editor/Node.hpp>

namespace Nebula::RenderGraph::Editor
{
    class GraphEditor
    {
        using Scene_t = sd::Scene;

    public:
        explicit GraphEditor(RenderGraphContext& context);

        void render();

        static void set_scene(const std::shared_ptr<Scene_t>& scene)
        {
            s_selected_scene = std::shared_ptr(scene);
        }

    private:
        void _handle_compile(Compiler::CompilerType mode);

        bool _handle_connection();

        void _erase_edge(int32_t edge_id);

        void _handle_add_node(NodeType type);

        void _handle_reset();

        void _add_default_nodes();

    public:
        static std::shared_ptr<Scene_t> s_selected_scene;

    private:
        std::map<int32_t, std::shared_ptr<Node>> m_nodes;
        std::vector<Edge>                        m_edges;
        std::vector<std::string>                 m_messages;

        bool m_has_scene_provider { false };
        bool m_has_presenter { false };

        RenderGraphContext& m_context;
    };
}