#include "Surface.hpp"

#include <GLFW/glfw3.h>

Surface::Surface(const Instance& instance)
: mInstance(instance)
{
    auto result = glfwCreateWindowSurface(mInstance.handle(), mInstance.window().handle(), nullptr, &mSurface);
}

Surface::~Surface()
{
    if (mSurface != nullptr)
    {
        vkDestroySurfaceKHR(mInstance.handle(), mSurface, nullptr);
    }
}
