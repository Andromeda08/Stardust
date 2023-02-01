#pragma once

#include <cstdint>

namespace sd
{
    struct RTAOParams
    {
        float   ao_radius {4.0f};       // Ray length
        int32_t ao_samples {64};        // Sample count per iteration
        float   ao_power {2.0f};        // Darkness strength per hit
        int32_t cause_shader {0};
        int32_t max_samples {100000};   // Max number of frames for accumulation.
        int32_t acc_frames {0};         // Current frame
    };
}