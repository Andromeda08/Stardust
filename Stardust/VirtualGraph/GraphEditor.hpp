#pragma once

#include <map>
#include <memory>
#include <vector>
#include <Utility.hpp>
#include <VirtualGraph/Node.hpp>
#include <VirtualGraph/Compile/GraphCompileStrategy.hpp>
#include <VirtualGraph/Common/GraphContext.hpp>
#include <Scene/Scene.hpp>

namespace Nebula::Editor
{
    class GraphEditor
    {
    private:
        using id_t = int32_t;
        using node_ptr_t = std::shared_ptr<Node>;
        using Scene_t = sd::Scene;

        struct UiEdge
        {
            id_t id = sd::util::gen_id();
            id_t start_node, start_attr, end_node, end_attr;
            ResourceType attr_type;

            UiEdge(id_t sn, id_t sa, id_t en, id_t ea, ResourceType at)
            : start_node(sn), start_attr(sa)
            , end_node(en), end_attr(ea)
            , attr_type(at)
            {}
        };

        std::map<id_t, node_ptr_t> m_nodes;
        std::vector<UiEdge>        m_edges;

        std::vector<std::string>              m_messages;
        std::unique_ptr<VirtualNodeFactory>   m_factory;
        std::unique_ptr<GraphCompileStrategy> m_compiler;

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