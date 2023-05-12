#pragma once

#include <array>
#include <string>
#include <imgui.h>
#include <Utility.hpp>

namespace sd::rg
{
    class Output
    {
    public:
        virtual const std::string& get_name() = 0;

        virtual const std::array<int32_t, 4>& get_color() = 0;

        ImU32 imu32()
        {
            const auto color = get_color();
            const auto imColor = ImColor(color[0], color[1], color[2], 255);
            return imColor.operator ImU32();
        }

        virtual ~Output() = default;

        int32_t id() const { return m_id; }

    protected:
        int32_t m_id { util::gen_id() };
    };
}