#include "GraphEditor.hpp"

#include <algorithm>
#include <format>
#include <iostream>
#include <imgui.h>
#include <imnodes.h>
#include <Application/Application.hpp>
#include <VirtualGraph/Compile/DefaultCompileStrategy.hpp>
#include <VirtualGraph/Compile/OptimizedCompileStrategy.hpp>

namespace Nebula::RenderGraph::Editor
{
    std::shared_ptr<sd::Scene> GraphEditor::s_selected_scene = nullptr;

    GraphEditor::GraphEditor(RenderGraphContext& context)
    : m_context(context)
    {
        _add_default_nodes();
    }

    void GraphEditor::render()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {16.f, 16.f});
        ImGui::Begin("RenderGraph Editor", nullptr, ImGuiWindowFlags_MenuBar);
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Add Node"))
                {
                    if (ImGui::MenuItem("Ambient Occlusion"))
                    {
                        _handle_add_node(NodeType::eAmbientOcclusion);
                    }
                    if (ImGui::MenuItem("Anti-Aliasing"))
                    {
                        _handle_add_node(NodeType::eAntiAliasing);
                    }
                    if (ImGui::MenuItem("Gaussian Blur"))
                    {
                        _handle_add_node(NodeType::eGaussianBlur);
                    }
                    if (ImGui::MenuItem("G-Buffer Pass"))
                    {
                        _handle_add_node(NodeType::ePrePass);
                    }
                    if (ImGui::MenuItem("Denoiser"))
                    {
                        _handle_add_node(NodeType::eDenoise);
                    }
                    if (ImGui::MenuItem("Lighting Pass"))
                    {
                        _handle_add_node(NodeType::eLightingPass);
                    }
                    if (ImGui::MenuItem("Present"))
                    {
                        if (m_has_presenter)
                        {
                            auto msg = std::format(R"([Warning] Only one instance of Present node is allowed.)");
                            m_messages.push_back(msg);
                            std::cout << msg << std::endl;
                        }
                        else
                        {
                            _handle_add_node(NodeType::ePresent);
                        }
                    }
                    if (ImGui::MenuItem("Ray Tracing"))
                    {
                        _handle_add_node(NodeType::eRayTracing);
                    }
                    if (ImGui::MenuItem("Scene Provider"))
                    {
                        if (m_has_presenter)
                        {
                            auto msg = std::format(R"([Warning] Only one instance of Scene Provider node is allowed.)");
                            m_messages.push_back(msg);
                            std::cout << msg << std::endl;
                        }
                        else
                        {
                            _handle_add_node(NodeType::eSceneProvider);
                        }
                    }
                    ImGui::EndMenu();
                }

                auto select_scene_text = std::format("Select Scene (Current: \"{}\")", s_selected_scene->name());
                if (ImGui::BeginMenu(select_scene_text.c_str()))
                {
                    if (ImGui::MenuItem("Default")) {}
                    ImGui::EndMenu();
                }

                if (ImGui::Button("Compile"))
                {
                    _handle_compile(Compiler::CompilerType::eNaiive);
                }

                if (ImGui::Button("Compile Optimzed"))
                {
                    _handle_compile(Compiler::CompilerType::eResourceOptimized);
                }

                if (ImGui::Button("Reset"))
                {
                    _handle_reset();
                }

                ImGui::EndMenuBar();
            }

            ImNodes::BeginNodeEditor();
            {
                auto [scale, font] = sd::Application::get_ui_scale();
                ImNodes::PushStyleVar(ImNodesStyleVar_PinCircleRadius, 4.0f * scale);
                for (const auto& node: m_nodes)
                {
                    node.second->render();
                }
                for (const auto& edge : m_edges)
                {
                    auto link_color = get_resource_type_color(edge.attr_type);
                    ImNodes::PushColorStyle(ImNodesCol_Link, IM_COL32(link_color.r, link_color.g, link_color.b, 255));
                    ImNodes::Link(edge.id, edge.start.res_id, edge.end.res_id);
                    ImNodes::PopColorStyle();
                }
            }
            ImNodes::EndNodeEditor();
        }

        _handle_connection();

        // _handle_link_delete();

        const int num_selected = ImNodes::NumSelectedLinks();
        if (num_selected > 0 && ImGui::IsKeyReleased(ImGuiKey_X))
        {
            std::vector<int32_t> selected_links;
            selected_links.resize(static_cast<size_t>(num_selected));
            ImNodes::GetSelectedLinks(selected_links.data());
            for (const int edge_id : selected_links)
            {
                _erase_edge(edge_id);
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void GraphEditor::_handle_compile(Compiler::CompilerType mode)
    {
        std::vector<node_ptr_t> nodes_vector;
        for (const auto& [k, v] : m_nodes)
        {
            nodes_vector.push_back(v);
        }

        std::unique_ptr<Compiler::GraphCompileStrategy> compiler;
        Compiler::CompileResult result;

        if (mode == Compiler::CompilerType::eNaiive)
        {
            compiler = std::make_unique<Compiler::DefaultCompileStrategy>(m_context);
        }

        if (mode == Compiler::CompilerType::eResourceOptimized)
        {
            compiler = std::make_unique<Compiler::OptimizedCompileStrategy>(m_context);
        }

        result = compiler->compile(nodes_vector, m_edges, true);

        for (const auto& msg : result.logs)
        {
            std::cout << msg << std::endl;
        }

        if (result.success)
        {
            m_context.set_render_path(result.render_path);
        }
    }

    bool GraphEditor::_handle_connection()
    {
        int32_t start_node, start_attr;
        int32_t end_node, end_attr;

        if (ImNodes::IsLinkCreated(&start_node, &start_attr, &end_node, &end_attr)) {
            try {
                const auto& attr = m_nodes[start_node]->get_resource(start_attr);
            }
            catch (const std::runtime_error& ex) {
                std::swap(start_node, end_node);
                std::swap(start_attr, end_attr);
            }

            auto& s_node = m_nodes[start_node];
            auto& s_attr = s_node->get_resource(start_attr);

            auto& e_node = m_nodes[end_node];
            auto& e_attr = e_node->get_resource(end_attr);

            auto edge_exists = std::any_of(m_edges.begin(), m_edges.end(), [s_attr, e_attr](const Edge& edge){
                return edge.start.res_id == s_attr.id && edge.end.res_id == e_attr.id;
            });

            std::string message;
            if (e_attr.input_is_connected)
            {
                message = std::format(R"([Error] The attribute "{}" of "{}" already has an input attached.)", e_attr.name, e_node->name());
                m_messages.push_back(message);
                std::cout << message << std::endl;
                return false;
            }

            if (edge_exists)
            {
                message = std::format(R"([Error] The attributes "{}" and "{}" are already connected.)", s_attr.name, e_attr.name);
                m_messages.push_back(message);
                std::cout << message << std::endl;
                return false;
            }

            if (s_attr.type != e_attr.type)
            {
                message = std::format(R"([Error] Type of attribute "{}" is not compatible with "{}".)", s_attr.name, e_attr.name);
                m_messages.push_back(message);
                std::cout << message << std::endl;
                return false;
            }

            Node::make_directed_edge(s_node, e_node);
            m_edges.emplace_back(*s_node, s_attr, *e_node, e_attr, s_attr.type);
            e_attr.input_is_connected = true;

            message = std::format(
                    R"([Info] Connecting: "{}" ("{}") --> "{}" ("{}"))",
                    s_node->name(), s_attr.name,
                    e_node->name(), e_attr.name
            );
            m_messages.push_back(message);
            std::cout << message << std::endl;
        }

        return true;
    }

    void GraphEditor::_handle_link_delete()
    {
        id_t link_id;
        if (ImNodes::IsLinkDestroyed(&link_id))
        {
            _erase_edge(link_id);
        }
    }

    void GraphEditor::_erase_edge(id_t edge_id)
    {
        auto edge = std::find_if(m_edges.begin(), m_edges.end(), [edge_id](const auto& edge){
            return edge.id == edge.id;
        });

        if (edge != m_edges.end())
        {
            auto& s_node = m_nodes[edge->start.node_id];
            auto& s_attr = s_node->get_resource(edge->start.res_id);

            auto& e_node = m_nodes[edge->end.node_id];
            auto& e_attr = e_node->get_resource(edge->end.res_id);
            e_attr.input_is_connected = false;

            auto find_a = std::find_if(s_node->get_outgoing_edges().begin(), s_node->get_outgoing_edges().end(), [&](const auto& v){
                return v->id() == e_node->id();
            });

            auto find_b = std::find_if(e_node->get_incoming_edges().begin(), e_node->get_incoming_edges().end(), [&](const auto& v){
                return v->id() == s_node->id();
            });

            auto erased_a = s_node->get_outgoing_edges().erase(find_a);
            auto erased_b = e_node->get_incoming_edges().erase(find_b);

            m_edges.erase(edge);

            std::cout
                << std::format(
                        R"([Info] Deleting link: "{}" ("{}") --> "{}" ("{}"))",
                        s_node->name(), s_attr.name,
                        e_node->name(), e_attr.name)
                << std::endl;
        }
    }

    void GraphEditor::_handle_add_node(NodeType type)
    {
        auto node = Node::Factory::create(type);
        m_nodes.insert({ node->id(), std::shared_ptr<Node>(node) });
    }

    void GraphEditor::_handle_reset()
    {
        m_messages.clear();
        m_nodes.clear();
        m_edges.clear();
        _add_default_nodes();
        std::cout << "[Info] Reset graph editor." << std::endl;
    }

    void GraphEditor::_add_default_nodes()
    {
        std::vector<NodeType> to_create = { NodeType::eSceneProvider, NodeType::ePresent, NodeType::ePrePass };

        for (auto t : to_create)
        {
            switch (t)
            {
                case NodeType::eSceneProvider:
                    m_has_scene_provider = true;
                    break;
                case NodeType::ePresent:
                    m_has_presenter = true;
                    break;
                default:
                    break;
            }

            auto node = Node::Factory::create(t);
            m_nodes.insert(std::pair<id_t, node_ptr_t>{
                node->id(),
                std::shared_ptr<Node>(node)
            });
        }
    }
}