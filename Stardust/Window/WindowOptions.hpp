#pragma once

#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <string>

namespace sd
{
    struct Extent
    {
        Extent() = default;
        Extent(uint32_t w, uint32_t h): width(w), height(h) {}

        uint32_t width { 0 };
        uint32_t height { 0 };

        vk::Extent2D vk_ext() const { return {width, height}; }
    };

    struct WindowOptions
    {
        Extent resolution { 1280, 720 };
        std::string title = "Title";
        bool fullscreen { false };

        void set_resolution(uint32_t width, uint32_t height)
        {
            resolution = {  width, height };
        }

        void set_title(std::string const& str) { title = str; }

        void set_fullscreen(bool b) { fullscreen = b; }

        uint32_t width() const { return resolution.width; }

        uint32_t height() const { return resolution.height; }
    };
}