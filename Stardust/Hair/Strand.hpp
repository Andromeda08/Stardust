#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <Resources/Geometry.hpp>
#include <Resources/VertexData.hpp>
#include <Scene/Transform.hpp>
#include <Vulkan/Buffer.hpp>
#include <Vulkan/Rendering/Mesh.hpp>

namespace Nebula
{
    using namespace sd;
    using namespace sdvk;

    struct HairSegment
    {
        uint32_t                     index  = 0;
        glm::vec3                    offset = {0, 0, 0};
        std::shared_ptr<HairSegment> parent = nullptr;

        HairSegment() = default;

        HairSegment(uint32_t i, glm::vec3 o, const std::shared_ptr<HairSegment>& p = nullptr)
        : index(i), offset(o), parent(p) {}

        glm::vec3 get_offset()
        {
            auto res = offset;
            if (parent) {
                res += parent->get_offset();
            }
            return res;
        }
    };

    class HairStrand : public Geometry
    {
    public:
        HairStrand(const glm::vec3& root, const std::vector<glm::vec3>& offsets)
        : m_root(root)
        {
            m_hair_segments = generate_segments(offsets);
            m_vertices = generate_vertices();
            for (uint32_t i = 0; i < offsets.size(); i++)
            {
                m_indices.push_back(i);
            }
        }

        static Geometry* make_test_strand()
        {
            std::vector<glm::vec3> offsets = {
                { 3, 1, 0 }, { -1, 2, 0 }, { -2, 4, 0 }
            };

            return new HairStrand({ 0, 0, 0 }, offsets);
        }

    private:
        std::vector<std::shared_ptr<HairSegment>> generate_segments(const std::vector<glm::vec3>& offsets)
        {
            std::vector<std::shared_ptr<HairSegment>> segments = { std::make_shared<HairSegment>(0, m_root) };
            for (const auto& offset : offsets)
            {
                auto& previous = segments.back();
                segments.push_back(std::make_shared<HairSegment>(segments.size(), offset, previous));
            }
            return segments;
        }

        std::vector<VertexData> generate_vertices()
        {
            std::vector<VertexData> vertices;
            for (const auto& segment : m_hair_segments)
            {
                vertices.push_back({ segment->get_offset(), { 0, 0, 0 }, { 0, 0 } });
            }
            return vertices;
        }

        std::vector<std::shared_ptr<HairSegment>>   m_hair_segments;
        glm::vec3                                   m_root;
    };
}