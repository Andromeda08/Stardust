#pragma once

#include <format>
// #include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cyCore.h>
#include <cyHairFile.h>
#include <glm/glm.hpp>
#include <Vulkan/Buffer.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>

namespace Nebula
{
    using namespace sdvk;

    struct HairVertex
    {
        glm::vec4 position;
    };

    struct Segment
    {
        uint32_t                id;
        uint32_t                point_count;
        std::vector<HairVertex> vertices;
    };

    class HairModel
    {
    public:
        explicit HairModel(const std::string& path): m_file(path)
        {
            m_hair_file.LoadFromFile(path.c_str());
            // std::cout << "Points            : " << m_hair_file.GetHeader().point_count << std::endl;
            // std::cout << "Strands           : " << m_hair_file.GetHeader().hair_count << "\n";

            process_vertices();
            // std::cout << "Processed Points  : " << m_vertices.size() << std::endl;

            process_segments();
            // std::cout << "Processed Strands : " << m_segments.size() << std::endl;
        }

        void create_vertex_buffer(const CommandBuffers& command_buffers, const Context& context)
        {
            m_vertex_buffer = Buffer::Builder()
                .with_name(std::format("[Hair] {} - Vertex Buffer", m_file))
                .with_size(sizeof(HairVertex) * m_vertices.size())
                .as_vertex_buffer()
                .create_with_data(m_vertices.data(), command_buffers, context);
        }

        const Buffer& vertex_buffer() const { return *m_vertex_buffer; }

        const Segment& get_segment(uint32_t id) const { return m_segments[id]; }

        const std::vector<Segment>& segments() const { return m_segments; }

    private:
        void process_vertices()
        {
            auto hair_points = m_hair_file.GetPointsArray();
            for (int32_t i = 0; i < m_hair_file.GetHeader().point_count * 3; i += 3)
            {
                m_vertices.emplace_back(glm::vec4(hair_points[i], hair_points[i + 1], hair_points[i + 2], 1.0f));
            }
        }

        void process_segments(bool segments_array = false)
        {
            if (segments_array)
            {
                std::cout << "Not yet" << std::endl;
            }

            uint32_t pts_per_seg = m_hair_file.GetHeader().d_segments + 1;
            for (uint32_t i = 0; i < m_hair_file.GetHeader().hair_count; i++)
            {
                Segment segment {};
                segment.id = i;
                segment.point_count = pts_per_seg;
                for (uint32_t j = (i * pts_per_seg); j < pts_per_seg; j++)
                {
                    segment.vertices.push_back(m_vertices[j]);
                }
                m_segments.push_back(segment);
            }
        }

        std::string              m_file;
        cyHairFile               m_hair_file;
        std::vector<HairVertex>  m_vertices;
        std::vector<Segment>     m_segments;
        std::shared_ptr<Buffer>  m_vertex_buffer;
    };
}