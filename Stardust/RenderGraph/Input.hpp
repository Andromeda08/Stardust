#pragma once
#include <memory>
#include <RenderGraph/Output.hpp>

namespace sd::rg
{
    class Input
    {
    public:
        // This function should decide whether or not an attached output is acceptable as input.
        virtual bool validate(std::shared_ptr<Output> const& incoming) { return false; }

        // Input name for UI.
        //virtual const std::string& get_name() = 0;

        // Input color for UI with a format of RGBA (0 - 255).
        virtual const std::array<float, 4>& get_color() = 0;

        virtual ~Input() = default;
    };
}