#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include <RenderGraph/Input.hpp>
#include <RenderGraph/Output.hpp>

namespace sd::rg
{
    class Node
    {
    public:
        virtual void compile() = 0;

        virtual void attach_input(std::shared_ptr<Output> const& output, std::shared_ptr<Input> const& input) {}

        virtual void execute(const vk::CommandBuffer& command_buffer) = 0;
    };
}