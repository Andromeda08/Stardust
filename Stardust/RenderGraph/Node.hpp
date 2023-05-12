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

        virtual ~Node() = default;

        virtual void compile() = 0;

        virtual void attach_input(std::shared_ptr<Output> const& output, std::shared_ptr<Input> const& input) {}

        virtual void execute(const vk::CommandBuffer& command_buffer) = 0;

        virtual void draw() {}

        Input& get_input(int32_t id)
        {
            for (const auto& input : m_inputs)
            {
                if (input->input_id() == id)
                {
                    return *input;
                }
            }

            throw std::runtime_error(std::string("[" + m_name + "] has no input with the ID " + std::to_string(id) + "."));
        }

        Output& get_output(int32_t id)
        {
            for (const auto& output : m_outputs)
            {
                if (output->output_id() == id)
                {
                    return *output;
                }
            }

            throw std::runtime_error(std::string("[" + m_name + "] has no output with the ID " + std::to_string(id) + "."));
        }

        int32_t id() const { return m_id; }

        const std::string& get_name() const { return m_name; }

        const ImColor& get_color() const { return m_ui_color; }

        const ImColor& get_hover_color() const { return m_ui_hover; };

    public:
        std::vector<std::unique_ptr<Input>>  m_inputs;
        std::vector<std::unique_ptr<Output>> m_outputs;

    protected:
        int32_t     m_id       { util::gen_id() };
        std::string m_name     {};
        ImColor     m_ui_color { 205, 214, 244, 255 };
        ImColor     m_ui_hover { 186, 194, 222, 255 };
    };
}