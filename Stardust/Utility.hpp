#pragma once

#include <random>

namespace sd::util
{
    static std::mt19937_64 engine(std::random_device{}());
    static std::uniform_int_distribution distribution;

    static int32_t gen_id()
    {
        return distribution(engine);
    }
}