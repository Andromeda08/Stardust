#pragma once

#include <map>
#include <memory>
#include <vector>
#include <Utility.hpp>
#include <VirtualGraph/Node.hpp>
#include <VirtualGraph/Compile/GraphCompileStrategy.hpp>
#include <Vulkan/Context.hpp>
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

        // TODO: Should replace Scene with a new representation
        std::shared_ptr<Scene_t> m_selected_scene;

        const sdvk::Context& m_context;

    public:
        explicit GraphEditor(const sdvk::Context& context);

        void render();

        void set_scene(const std::shared_ptr<Scene_t>& scene)
        {
            m_selected_scene = std::shared_ptr<Scene_t>(scene);
        }

    private:
        void _handle_compile();

        bool _handle_connection();

        void _handle_link_delete();

        void _erase_edge(id_t edge_id);
    };
}