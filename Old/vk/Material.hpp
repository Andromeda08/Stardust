#pragma

#include <glm/glm.hpp>

struct Material
{
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    glm::vec4 shininess;
};

// Values from: http://www.it.hiof.no/~borres/j3d/explain/light/p-materials.html
namespace Materials
{
    const Material ruby = {
        .ambient = glm::vec4(0.1745f, 0.01175f, 0.01175f, 0.55f),
        .diffuse = glm::vec4(0.61424f, 0.04136f, 0.04136f, 0.55f),
        .specular = glm::vec4(0.727811f, 0.626959f, 0.626959f, 0.55f),
        .shininess = glm::vec4(76.8f)
    };
    const Material pearl = {
        .ambient = glm::vec4(0.25f, 0.20725f, 0.20725f, 0.922f),
        .diffuse = glm::vec4(1.0f, 0.829f, 0.829f, 0.922f),
        .specular = glm::vec4(0.296648f, 0.296648f, 0.296648f, 0.922f),
        .shininess = glm::vec4(11.264f)
    };
    const Material purple = {
        .ambient = glm::vec4(0.105882f, 0.058824f, 0.113725f, 1.0f),
        .diffuse = glm::vec4(0.427451f, 0.470588f, 0.541176f, 1.0f),
        .specular = glm::vec4(0.333333f, 0.333333f, 0.521569f, 1.0f),
        .shininess = glm::vec4(9.84615f)
    };
    const Material obsidian = {
        .ambient = glm::vec4(0.05375f, 0.05f, 0.06625f, 0.82f),
        .diffuse = glm::vec4(0.18275f, 0.17f, 0.22525f, 0.82f),
        .specular = glm::vec4(0.332741f, 0.328634f, 0.346435f, 0.82f),
        .shininess = glm::vec4(38.4f)
    };
    const Material gold = {
        .ambient = glm::vec4(0.24725f, 0.1995f, 0.0745f, 1.0f),
        .diffuse = glm::vec4(0.75164f, 0.60648f, 0.22648f, 1.0f),
        .specular = glm::vec4(0.628281f, 0.555802f, 0.366065f, 1.0f),
        .shininess = glm::vec4(51.2f)
    };
    const Material emerald = {
        .ambient   = { 0.0215f, 0.1745f, 0.0215f, 0.55f },
        .diffuse   = { 0.07568f, 0.61424f, 0.07568f, 0.55f },
        .specular  = { 0.633f, 0.727811f, 0.633f, 0.55f },
        .shininess = glm::vec4(76.8f)
    };
    const Material cyan = {
        .ambient   = { 0.1f, 0.18725f, 0.1745f, 0.8f },
        .diffuse   = { 0.396f, 0.74151f, 0.69102f, 0.8f },
        .specular  = { 0.297254f, 0.30829f, 0.306678f, 0.8f },
        .shininess = glm::vec4(12.8f)
    };
}