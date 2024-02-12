#include <cstdint>
#include <format>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <Math/Graph/Vertex.hpp>

using namespace Nebula::Graph;

void print_vertices(std::map<int32_t, std::shared_ptr<Vertex>>& vertices)
{
    for (const auto& [ id, vertex ] : vertices)
    {
        std::stringstream sstr_in, sstr_out;
        for (const auto& in : vertex->get_incoming_edges())
        {
            sstr_in << std::format(" {} ", in->name());
        }
        for (const auto& out : vertex->get_outgoing_edges())
        {
            sstr_out << std::format(" {} ", out->name());
        }
        std::cout << std::format("Vertex {}: To: [{}]\tFrom:[{}]", vertex->name(), sstr_out.str(), sstr_in.str()) << std::endl;
    }
}

int main()
{
    std::map<int32_t, std::shared_ptr<Vertex>> vertices;

    vertices.insert({ 0, std::make_unique<Vertex>("0") });
    vertices.insert({ 1, std::make_unique<Vertex>("1") });
    vertices.insert({ 2, std::make_unique<Vertex>("2") });

    Vertex::make_directed_edge(vertices[0], vertices[1]);
    Vertex::make_directed_edge(vertices[0], vertices[2]);

    std::cout << "Vertices:" << std::endl;
    print_vertices(vertices);

    Vertex::delete_directed_edge(vertices[0], vertices[2]);

    std::cout << "Vertices:" << std::endl;
    print_vertices(vertices);

    return 0;
}