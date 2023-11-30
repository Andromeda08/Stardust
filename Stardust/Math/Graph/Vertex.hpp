#pragma once

#include <vector>
#include <string>
#include <uuid.h>
#include <Utility.hpp>

namespace Nebula::Graph
{
    class Vertex
    {
        using vtx_ptr = std::shared_ptr<Vertex>;
    public:
        /**
         * Create a directed edge from A to B
         * \return success bool
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

        /**
         * Remove directed edge from A to B
         * \return success bool
         */
        static bool delete_directed_edge(const vtx_ptr& a, const vtx_ptr& b)
        {
            if (a->uuid() == b->uuid()) return false;

            auto& a_out = a->get_outgoing_edges();
            const auto b_find = std::ranges::find_if(a_out, [&](auto& v){ return v->id() == b->id(); });
            if (b_find == std::end(a_out)) return false;

            auto& b_in = b->get_incoming_edges();
            const auto a_find = std::ranges::find_if(b_in, [&](auto& v){ return v->id() == a->id(); });
            if (a_find == std::end(b_in)) return false;

            a_out.erase(b_find);
            b_in.erase(a_find);

            return true;
        }

        virtual ~Vertex() = default;

        explicit Vertex(const std::string& name): m_name(name) {}

        [[nodiscard]] const uuids::uuid& uuid() const { return m_uuid; }

        [[nodiscard]] int32_t id() const { return m_id; }

        [[nodiscard]] const std::string& name() const { return m_name; }

        [[nodiscard]] std::vector<vtx_ptr>& get_incoming_edges()
        {
            return m_incoming_edges;
        }

        [[nodiscard]] int32_t in_degree() const
        {
            return static_cast<int32_t>(m_incoming_edges.size());
        }

        [[nodiscard]] std::vector<vtx_ptr>& get_outgoing_edges()
        {
            return m_outgoing_edges;
        }

        [[nodiscard]] int32_t out_degree() const
        {
            return static_cast<int32_t>(m_outgoing_edges.size());
        }

    private:
        int32_t     m_id   = sd::util::gen_id();
        uuids::uuid m_uuid = uuids::uuid_system_generator{}();
        std::string m_name = "Unknown Node";

        std::vector<vtx_ptr> m_incoming_edges;
        std::vector<vtx_ptr> m_outgoing_edges;
    };
}