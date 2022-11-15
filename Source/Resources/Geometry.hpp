#pragma once

#include <vk/VertexData.hpp>
#include <glm/glm.hpp>

struct InstanceData
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 scale;
};

class Geometry
{
public:
    Geometry() = default;

    const std::vector<re::VertexData>& vertices() { return m_vertices; }
    const std::vector<uint32_t>& indices() { return m_indices; }

protected:
    std::vector<re::VertexData>   m_vertices;
    std::vector<uint32_t> m_indices;
};

class SphereGeometry : public Geometry
{
public:
    SphereGeometry(float radius = 1.0f, glm::vec3 color = { 0.5f, 0.5f, 0.5f }, int resolution = 40)
        : Geometry()
    {
        this->m_vertices = gen_vertices(resolution, resolution, radius, color);
        this->m_indices  = gen_indices(resolution, resolution);
    }

private:
    static std::vector<re::VertexData> gen_vertices(int stack_count, int sector_count, float radius = 1.0f, glm::vec3 color = { 0.5f, 0.5f, 0.5f })
    {
        std::vector<glm::vec3> vertices, colors, normals;
        std::vector<glm::vec2> uvs;

        float sector_step = 2 * M_PI / sector_count;
        float stack_step  = M_PI / stack_count;
        float sector_angle, stack_angle;

        float r_inv = 1.0f / radius;

        for (int i = 0; i <= stack_count; i++)
        {
            float x, y, z, xy;
            stack_angle = M_PI / 2 - (float) i * stack_step;
            xy = radius * cosf(stack_angle);
            z = radius * sinf(stack_angle);

            for (int j = 0; j <= sector_count; j++)
            {
                sector_angle = (float) j * sector_step;

                x = xy * cosf(sector_angle);
                y = xy * sinf(sector_angle);

                vertices.emplace_back(x, y, z);
                normals.emplace_back(x * r_inv, y * r_inv, z * r_inv);
                uvs.emplace_back((float) j / sector_count, (float) i / stack_count);
                colors.emplace_back(color);
            }
        }

        std::vector<re::VertexData> result;
        for (uint32_t i = 0; i < vertices.size(); i++)
        {
            result.push_back({ vertices[i], colors[i], normals[i],uvs[i] });
        }
        return result;
    }

    static std::vector<uint32_t> gen_indices(int stack_count, int sector_count)
    {
        std::vector<uint32_t> indices;
        uint32_t k1, k2;

        for (int i = 0; i < stack_count; i++)
        {
            k1 = i * (sector_count + 1);
            k2 = k1 + sector_count + 1;

            for (int j = 0; j < sector_count; j++, k1++, k2++)
            {
                if (i != 0)
                {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }
                if (i != (stack_count - 1))
                {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
        }
        return indices;
    }
};

class CubeGeometry : public Geometry
{
public:
    CubeGeometry(float scale = 1.0f, glm::vec3 color = { 0.5f, 0.5f, 0.5f })
        : Geometry()
    {
        this->m_vertices = gen_vertices(scale, color);
        this->m_indices  = gen_indices();
    }

private:
    static std::vector<re::VertexData> gen_vertices(float scale = 1.0f, glm::vec3 color = { 0.5f, 0.5f, 0.5f })
    {
        std::vector<re::VertexData> result = {
            {{-1.0f, -1.0f, -1.0f}, color, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
            {{-1.0f, 1.0f, -1.0f}, color, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
            {{1.0f, 1.0f, -1.0f}, color, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
            {{1.0f, -1.0f, -1.0f}, color, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
            {{-1.0f, -1.0f, 1.0f}, color, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{1.0f, -1.0f, 1.0f}, color, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{1.0f, 1.0f, 1.0f}, color, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-1.0f, 1.0f, 1.0f}, color, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            {{-1.0f, -1.0f, -1.0f}, color, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{-1.0f, -1.0f, 1.0f}, color, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{-1.0f, 1.0f, 1.0f}, color, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{-1.0f, 1.0f, -1.0f}, color, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
            {{1.0f, -1.0f, 1.0f}, color, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{1.0f, -1.0f, -1.0f}, color, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{1.0f, 1.0f, -1.0f}, color, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{1.0f, 1.0f, 1.0f}, color, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
            {{-1.0f, -1.0f, -1.0f}, color, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{1.0f, -1.0f, -1.0f}, color, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{1.0f, -1.0f, 1.0f}, color, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{-1.0f, -1.0f, 1.0f}, color, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
            {{-1.0f, 1.0f, 1.0f}, color, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
            {{1.0f, 1.0f, 1.0f}, color, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
            {{1.0f, 1.0f, -1.0f}, color, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
            {{-1.0f, 1.0f, -1.0f}, color, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        };

        for (auto& v : result)
        {
            v.position *= scale;
        }

        return result;
    }

    static std::vector<uint32_t> gen_indices()
    {
        return {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            8, 9, 10, 10, 11, 8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
        };
    }
};