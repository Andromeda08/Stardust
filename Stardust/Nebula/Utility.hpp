#pragma once

#include <format>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace Nebula
{
    class Utility
    {
    public:
        template <typename E = std::runtime_error>
        static E make_exception(const std::string& message) noexcept
        {
            static_assert(std::is_base_of_v<std::exception, E>, "E must be a type of std::exception");
            return E(std::format("{} {}", s_error_prefix, message));
        }

    private:
        static constexpr std::string s_error_prefix = "[Error]";
    };
}