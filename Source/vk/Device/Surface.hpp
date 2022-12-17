#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include "Instance.hpp"
#include <Utility/Macro.hpp>

class Surface {
public:
    NON_COPIABLE(Surface)

    explicit Surface(const Instance& instance);

    ~Surface();

    VkSurfaceKHR handle() const { return mSurface; }

    const Instance& instance() const { return mInstance; }

private:
    VkSurfaceKHR    mSurface;
    const Instance& mInstance;
};
