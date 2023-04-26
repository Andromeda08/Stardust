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

            Builder& accept_formats(const std::set<vk::Format>& formats)
            {
                parameters.accepted_formats = formats;
                return *this;
            }

            // Sets accepted formats to common color formats.
            //Builder& expect_color_image();

            // Sets accepted formats to depth formats.
            //Builder& expect_depth_image();

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

                return std::make_unique<ImageResource>(parameters);
            }

            std::unique_ptr<ImageResource> create_from_resource(const std::shared_ptr<sdvk::Image>& resource)
            {
                parameters.min_resolution = parameters.max_resolution = resource->extent();
                parameters.accepted_formats = { resource->format() };

                auto result = std::make_unique<ImageResource>(parameters);
                result->m_resource = resource;

                return result;
            }

            Parameters parameters;
        };

        explicit ImageResource(Parameters parameters): m_parameters(std::move(parameters)) {}

        bool validate(const std::shared_ptr<Output>& incoming) override
        {
            auto res = std::dynamic_pointer_cast<ImageResource>(incoming);

            return true;
        }

        const std::array<float, 4>& get_color() override { return m_ui_color; }

        std::shared_ptr<sdvk::Image> m_resource { nullptr };
        Parameters                   m_parameters {};

    private:
        const std::array<float, 4>   m_ui_color { 137.f, 220.f, 235.f, 255.f };
    };

//    std::ostream& operator<<(std::ostream& os, const ImageResource& res)
//    {
//        const auto& e_max = res.m_parameters.max_resolution;
//        const auto& e_min = res.m_parameters.min_resolution;
//
//        os << "ImageResource:\n"
//        << "\tMin. Extent : " << e_min.width << "x" << e_min.height << "\n"
//        << "\tMax. Extent : " << e_max.height << "x" << e_max.height << "\n";
//
//        if (res.m_resource != nullptr)
//        {
//            auto e = res.m_resource->extent();
//
//            os << "\tFormat : " << to_string(res.m_resource->format()) << "\n";
//            os << "\tExtent : " << e.width << "x" << e.height << "\n";
//        }
//
//        return os;
//    }
}