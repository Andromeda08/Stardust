#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <imnodes.h>
#include <RenderGraph/Node.hpp>
#include <RenderGraph/Scene.hpp>

namespace sd::rg
{
    class RenderGraphEditor
    {
    public:
        struct Edge
        {
            int32_t id;
            int32_t from, to;
            int32_t from_node, to_node;

            Edge() = default;
            Edge(int32_t id, int32_t fn, int32_t f, int32_t tn, int32_t t)
            : id(id), from_node(fn), from(f), to_node(tn), to(t) {}
        };

        RenderGraphEditor(const sdvk::CommandBuffers& command_buffers,
                          const sdvk::Context& context,
                          const sdvk::Swapchain& swapchain,
                          const std::shared_ptr<Scene>& scene);

        void draw();

    private:
        std::vector<int32_t> _topological_sort();

        void _add_initial_nodes();

    private:
        std::set<std::string> m_node_types { "OffscreenRender", "RTAO" };

        std::shared_ptr<Scene> m_scene;

        int32_t m_current_id {0};
        std::map<int32_t, std::unique_ptr<Node>> m_nodes;
        std::map<int32_t, std::vector<int32_t>>  m_adjacency_list;
        std::map<int32_t, int32_t>               m_in_degree;
        std::vector<Edge>                        m_edges;

        const sdvk::CommandBuffers& m_command_buffers;
        const sdvk::Context& m_context;
        const sdvk::Swapchain& m_swapchain;
    };
}
