#include "RenderGraphEditor.hpp"

#include <iostream>
#include <stack>
#include <sstream>
#include <Benchmarking.hpp>
#include <RenderGraph/node/CompositionNode.hpp>
#include <RenderGraph/node/OffscreenRenderNode.hpp>
#include <RenderGraph/node/SceneNode.hpp>
#include <RenderGraph/node/RTAONode.hpp>

namespace sd::rg
{
    RenderGraphEditor::RenderGraphEditor(const sdvk::CommandBuffers& command_buffers,
                                         const sdvk::Context& context,
                                         const sdvk::Swapchain& swapchain,
                                         const std::shared_ptr<Scene>& scene)
    : m_command_buffers(command_buffers)
    , m_context(context)
    , m_swapchain(swapchain)
    , m_scene(scene)
    {
        _add_initial_nodes();
    }

    void RenderGraphEditor::draw()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 12.f, 12.f });
        ImGui::Begin("Render Graph Editor");

        ImNodes::BeginNodeEditor();

        if (ImGui::Button("Sort"))
        {
            auto order = _topological_sort();
            #ifdef SD_DEBUG
            std::stringstream strs;
            for (const auto& i : order)
            {
                strs << "[" + m_nodes[i]->get_name() + " | " + std::to_string(m_nodes[i]->id()) + "] ";
            }
            std::cout << "Topological Ordering:\n" << strs.str() << std::endl;
            #endif
        }

        if (ImGui::Button("Compile"))
        {
            compile();
        }

        for (const auto& pair : m_nodes)
        {
            pair.second->draw();
        }

        for (const auto& edge : m_edges)
        {
            ImNodes::Link(edge.id, edge.from, edge.to);
        }

        ImNodes::EndNodeEditor();

        int32_t start_node, start_attr, end_node, end_attr;
        if (ImNodes::IsLinkCreated(&start_node, &start_attr, &end_node, &end_attr))
        {
            try {
                const auto& attr = m_nodes[start_node]->get_output(start_attr);
            }
            catch (const std::runtime_error& ex) {
                std::swap(start_node, end_node);
                std::swap(start_attr, end_attr);
            }

            auto& sattr = m_nodes[start_node]->get_output(start_attr);
            auto& eattr = m_nodes[end_node]->get_input(end_attr);

            auto is_valid_connection = eattr.validate(sattr);

            if (is_valid_connection)
            {
                auto id = util::gen_id();
                m_edges.emplace_back(id, start_node, start_attr, end_node, end_attr);
                m_in_degree[end_node]++;
                m_adjacency_list[start_node].push_back(end_node);

                std::cout
                        << "Connected ["
                        << sattr.output_id() << " of " << m_nodes[start_node]->get_name() << " (" << m_nodes[start_node]->id()
                        << ")] to ["
                        << eattr.input_id() << " of " << m_nodes[end_node]->get_name() << " (" << m_nodes[end_node]->id()
                        << ")]"
                        << std::endl;
            }
            else
            {
                std::cout
                    << "Invalid connection: ["
                    << eattr.input_id() << " of " << m_nodes[end_node]->get_name() << " (" << m_nodes[end_node]->id()
                    << ")] received unexpected resource type."
                    << std::endl;
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void RenderGraphEditor::compile()
    {
        auto compile_time = bm::measure([&](){
            m_render_path = _topological_sort();

            for (const auto& edge : m_edges)
            {
                auto& output = m_nodes[edge.from_node]->get_output(edge.from);
                auto& input = m_nodes[edge.to_node]->get_input(edge.to);

                input.validate(output);

                input.link_output(output);
            }

            for (const auto& node_id : m_render_path)
            {
                m_nodes[node_id]->compile();
            }
        });

        std::cout << "Compiled graph in " << compile_time.count() << "ms" << std::endl;
        std::stringstream strs;
        for (const auto& i : m_render_path)
        {
            strs << "[" + m_nodes[i]->get_name() + " | " + std::to_string(m_nodes[i]->id()) + "] ";
        }
        std::cout << "Topological Ordering:\n" << strs.str() << std::endl;
    }

    bool RenderGraphEditor::execute(const vk::CommandBuffer& command_buffer)
    {
        if (m_render_path.empty())
        {
            return false;
        }

        for (const auto& node_id : m_render_path)
        {
            m_nodes[node_id]->execute(command_buffer);
        }
        return true;
    }

    void RenderGraphEditor::_add_initial_nodes()
    {
        std::unique_ptr<Node> temp;
        temp = std::make_unique<SceneNode>(m_scene);
        m_nodes[temp->id()] = std::move(temp);

        temp = std::make_unique<CompositionNode>(m_command_buffers, m_context, m_swapchain, *this);
        m_nodes[temp->id()] = std::move(temp);

        temp = std::make_unique<OffscreenRenderNode>(m_context, m_command_buffers);
        m_nodes[temp->id()] = std::move(temp);

        temp = std::make_unique<RTAONode>(m_context, m_command_buffers);
        m_nodes[temp->id()] = std::move(temp);
    }

    std::vector<int32_t> RenderGraphEditor::_topological_sort()
    {
        std::vector<int32_t> T;
        std::stack<int32_t>  S;
        auto in_degree = m_in_degree;

        for (const auto& n : m_nodes)
        {
            if (in_degree[n.first] == 0)
            {
                S.push(n.first);
            }
        }

        while (!S.empty())
        {
            auto n = S.top();
            S.pop();

            T.push_back(n);

            for (auto m : m_adjacency_list[n])
            {
                in_degree[m]--;
                if (in_degree[m] == 0)
                {
                    S.push(m);
                }
            }
        }

        for (const auto& n : m_nodes)
        {
            if (in_degree[n.first] != 0)
            {
                throw std::logic_error("Graph isn't acyclic.");
            }
        }

        return T;
    }
}