#pragma once

#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <string>

namespace sd
{
    enum class Resolution
    {
        e1280x720,
        e1600x900,
        e1920x1080,
        e2560x1440,
        e3840x2160
    };

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

        void set_resolution(Resolution res)
        {
            switch (res) {
                case Resolution::e1280x720:
                    resolution = { 1280, 720 };
                    break;
                case Resolution::e1600x900:
                    resolution = { 1600, 900 };
                    break;
                case Resolution::e1920x1080:
                    resolution = { 1920, 1080 };
                    break;
                case Resolution::e2560x1440:
                    resolution = { 2560, 1440 };
                    break;
                case Resolution::e3840x2160:
                    resolution = { 3840, 2160 };
                    break;
            }
        }

        void set_title(std::string const& str) { title = str; }

        void set_fullscreen(bool b) { fullscreen = b; }

        uint32_t width() const { return resolution.width; }

        uint32_t height() const { return resolution.height; }
    };
}