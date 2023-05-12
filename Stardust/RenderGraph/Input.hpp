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
        // This function should decide whether an attached output is acceptable as input.
        virtual bool validate(std::shared_ptr<Output> const& incoming) { return false; }

        // Input name for UI.
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

        int32_t id() const { return m_id; }

    protected:
        int32_t m_id { util::gen_id() };
    };
}