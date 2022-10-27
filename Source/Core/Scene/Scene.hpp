#pragma once

#include <vector>
#include "../Vulkan/Device.hpp";
#include "../Vulkan/Instance.hpp";

class Scene
{
public:

    Scene()  = default;
    ~Scene() = default;

    void setup(const Instance& instance,const Device& device, uint32_t queue_family);

    void create_descriptor_set_layout();

    void create_graphics_pipeline();

    void update_descriptor_set();

    void create_uniform_buffer();

    void update_uniform_buffer();

    void rasterize(const vk::CommandBuffer& cmd);


private:
};
