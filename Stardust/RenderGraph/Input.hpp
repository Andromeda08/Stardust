#pragma once
#include <memory>
#include <RenderGraph/Output.hpp>

namespace sd::rg
{
    class Input
    {
    public:
        virtual bool validate(std::shared_ptr<Output> const& incoming) { return false; }
    };
}