#pragma once

#include <iostream>
#include <memory>
#include <set>
#include <utility>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Image/Image.hpp>
#include <RenderGraph/Input.hpp>
#include <RenderGraph/Output.hpp>

namespace sd::rg
{
    struct ImageResource : public Input, public Output
    {
        struct Parameters
        {
            std::set<vk::Format>    accepted_formats {};
            vk::Extent2D            min_resolution { 1, 1 };
            vk::Extent2D            max_resolution { 8192, 8192};
            vk::SampleCountFlagBits sample_count { vk::SampleCountFlagBits::e1 };
        };

        struct Builder
        {
            Builder() = default;

            Builder& with_name(std::string&& name)
            {
                _name = name;
                return *this;
            }

            Builder& accept_formats(const std::set<vk::Format>& formats)
            {
                parameters.accepted_formats = formats;
                return *this;
            }

            Builder& with_min_resolution(vk::Extent2D&& min_res)
            {
                parameters.min_resolution = min_res;
                return *this;
            }

            Builder& with_max_resolution(vk::Extent2D&& max_res)
            {
                parameters.max_resolution = max_res;
                return *this;
            }

            Builder& with_sample_count(vk::SampleCountFlagBits samples)
            {
                parameters.sample_count = samples;
                return *this;
            }

            std::unique_ptr<ImageResource> create()
            {
                if (parameters.accepted_formats.empty())
                {
                    throw std::runtime_error("ImageResource must have at least 1 acceptable format defined.");
                }

                if (parameters.min_resolution > parameters.max_resolution)
                {
                    throw std::runtime_error("The minimums resolution of an ImageResource must be less than its max resolution");
                }

                return std::make_unique<ImageResource>(parameters, _name);
            }

            std::unique_ptr<ImageResource> create_from_resource(const std::shared_ptr<sdvk::Image>& resource)
            {
                parameters.min_resolution = parameters.max_resolution = resource->extent();
                parameters.accepted_formats = { resource->format() };

                auto result = std::make_unique<ImageResource>(parameters, _name);
                result->m_resource = resource;

                return result;
            }

            Parameters parameters;
        private:
            std::string _name {};
        };

        explicit ImageResource(Parameters parameters, std::string name)
        : m_parameters(std::move(parameters))
        , m_ui_name(std::move(name)) {}

        bool validate(const std::shared_ptr<Output>& incoming) override
        {
            auto res = std::dynamic_pointer_cast<ImageResource>(incoming);

            return true;
        }

        const std::array<int32_t, 4>& get_color() override { return m_ui_color; }

        const std::string& get_name() override { return m_ui_name; }

        std::shared_ptr<sdvk::Image> m_resource { nullptr };
        Parameters                   m_parameters {};

    private:
        const std::array<int32_t, 4> m_ui_color { 203, 166, 247, 255 };
        const std::string m_ui_name {};
    };
}