#pragma once
#include <string>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Image/Image.hpp>

namespace sd::rg
{
    class Node
    {
    public:
        using resource_t = sdvk::Image;

        virtual void execute(const vk::CommandBuffer& cmd) {}

        virtual void wire_input(const std::string& location, const std::string& from, const Node& src) {}

    protected:
        static constexpr auto s_depth_validator = [](const resource_t& res) {
            std::set<vk::Format> accepted_formats = {
                    vk::Format::eD16Unorm,  vk::Format::eD16UnormS8Uint,  vk::Format::eD24UnormS8Uint,
                    vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eX8D24UnormPack32
            };

            return accepted_formats.contains(res.format());
        };

        static constexpr auto s_image_validator = [](const resource_t& res){
            std::set<vk::Format> accepted_formats = {
                    vk::Format::eR32G32B32A32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat, vk::Format::eR32Sfloat,
                    vk::Format::eR16G16B16A16Sfloat, vk::Format::eR16G16B16Sfloat, vk::Format::eR16G16Sfloat, vk::Format::eR16Sfloat,
                    vk::Format::eR8G8B8A8Unorm,      vk::Format::eR8G8B8Unorm,     vk::Format::eR8G8Unorm,    vk::Format::eR8Unorm
            };

            return accepted_formats.contains(res.format());
        };
    };
}