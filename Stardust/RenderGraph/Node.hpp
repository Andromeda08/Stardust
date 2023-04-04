#pragma once
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Input.hpp"
#include "Output.hpp"

namespace sd::rg
{
    class INode
    {
    public:
        virtual void compile() {}
        virtual void attach_input(std::shared_ptr<IOutput> const& output, std::shared_ptr<IInput> const& input) {}
        virtual void execute(vk::CommandBuffer command_buffer) {}
    };
}