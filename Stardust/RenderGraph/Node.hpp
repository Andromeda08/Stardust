#pragma once

#include <memory>
#include <imgui.h>
#include <Utility.hpp>
#include <vulkan/vulkan.hpp>
#include <RenderGraph/Input.hpp>
#include <RenderGraph/Output.hpp>

namespace sd::rg
{
    class Node
    {
    public:
        Node() = default;

        explicit Node(std::string&& name): m_name(name) {}

        Node(ImColor color, ImColor hover_color): m_ui_color(color), m_ui_hover(hover_color) {}

        Node(std::string&& name, ImColor color, ImColor hover_color): m_name(name), m_ui_color(color), m_ui_hover(hover_color) {}

        virtual void compile() = 0;

        virtual void attach_input(std::shared_ptr<Output> const& output, std::shared_ptr<Input> const& input) {}

        virtual void execute(const vk::CommandBuffer& command_buffer) = 0;

        virtual void draw() {}

        virtual const Input& get_input(int32_t id) = 0;

        virtual const Output& get_output(int32_t id) = 0;

        int32_t id() const { return m_id; }

        const std::string& get_name() const { return m_name; }

        const ImColor& get_color() const { return m_ui_color; }

        const ImColor& get_hover_color() const { return m_ui_hover; };

        virtual ~Node() = default;

    protected:
        int32_t     m_id       { util::gen_id() };
        std::string m_name     {};
        ImColor     m_ui_color { 205, 214, 244, 255 };
        ImColor     m_ui_hover { 186, 194, 222, 255 };
    };
}