#pragma once

#include <chrono>
#include <functional>

namespace sd::bm
{
    using clock = std::chrono::high_resolution_clock;

    static const auto start_time = clock::now();

    template <typename unit_t = std::chrono::seconds>
    static unit_t measure(const std::function<void()>& fn)
    {
        const auto start = clock::now();
        fn();
        const auto end = clock::now();

        return duration_cast<unit_t>(end - start);
    }
}