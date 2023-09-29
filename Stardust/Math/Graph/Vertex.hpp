#pragma once

#include <vector>
#include <string>
#include <uuid.h>
#include <Utility.hpp>

namespace Nebula::Graph
{
    class Vertex
    {
    private:
        using vtx_ptr = std::shared_ptr<Vertex>;

        int32_t     m_id   = sd::util::gen_id();
        uuids::uuid m_uuid = uuids::uuid_system_generator{}();
        std::string m_name = "Unknown Node";

        std::vector<vtx_ptr> m_incoming_edges;
        std::vector<vtx_ptr> m_outgoing_edges;

    public:
        explicit Vertex(const std::string& name): m_name(name) {}

        const uuids::uuid& uuid() const { return m_uuid; }

        int32_t id() const { return m_id; }

        const std::string& name() const { return m_name; }

        std::vector<vtx_ptr>& get_incoming_edges()
        {
            return m_incoming_edges;
        }

        [[nodiscard]] int32_t in_degree() const
        {
            return static_cast<int32_t>(m_incoming_edges.size());
        }

        std::vector<vtx_ptr>& get_outgoing_edges()
        {
            return m_outgoing_edges;
        }

        [[nodiscard]] int32_t out_degree() const
        {
            return static_cast<int32_t>(m_outgoing_edges.size());
        }

        /**
         * Create a directed edge from A to B
         */
        static bool make_directed_edge(const vtx_ptr& a, const vtx_ptr& b)
        {
            if (a->uuid() == b->uuid())
            {
                return false;
            }

            a->m_outgoing_edges.push_back(b);
            b->m_incoming_edges.push_back(a);

            return true;
        }

        static bool remove_directed_edge(const vtx_ptr& a, const vtx_ptr& b)
        {
            auto find_a = std::find_if(a->m_outgoing_edges.begin(), a->m_outgoing_edges.end(), [&](const auto& v){
                return v->id() == b->id();
            });
            if (find_a == std::end(a->m_outgoing_edges))
            {
                return false;
            }

            auto find_b = std::find_if(b->m_incoming_edges.begin(), b->m_incoming_edges.end(), [&](const auto& v){
                return v->id() == a->id();
            });
            if (find_b == std::end(b->m_incoming_edges))
            {
                return false;
            }

            auto erased_a = a->m_outgoing_edges.erase(find_a);
            auto erased_b = b->m_incoming_edges.erase(find_b);

            return true;
        }

        /**
         * Remove directed edge from A to B
         */
        static bool delete_directed_edge(const vtx_ptr& a, const vtx_ptr& b)
        {
            return false;
        }
    };
}