#pragma once

#include <array>
#include <memory>
#include <imgui.h>
#include <Utility.hpp>
#include <RenderGraph/Output.hpp>

namespace sd::rg
{
    class Input
    {
    public:
        virtual void link_output(Output& input) = 0;


        virtual bool validate(const Output& output) const { return false; }

        virtual const std::string& get_name() = 0;

        // Input color for UI with a format of RGBA (0 - 255).
        virtual const std::array<int32_t, 4>& get_color() = 0;

        ImU32 imu32()
        {
            const auto color = get_color();
            const auto imColor = ImColor(color[0], color[1], color[2], 255);
            return imColor.operator ImU32();
        }

        virtual ~Input() = default;

        int32_t input_id() const { return m_input_id; }

    protected:
        int32_t m_input_id { util::gen_id() };
    };
}