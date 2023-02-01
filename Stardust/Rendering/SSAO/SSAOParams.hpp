#pragma once

#include <cstdint>

namespace sd
{
    struct SSAOParams
    {
        int32_t ao_noise_dim {4};
        float   ao_radius {0.3f};
        int32_t ao_kernel_size {64};
    };
}