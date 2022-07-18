#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <QuartzCore/CAMetalLayer.hpp>

namespace krypton::rapi::mtl {
    void setMetalLayerOnWindow(GLFWwindow* window, CA::MetalLayer* layer);

    // Returns 'true' if the display that is currently displaying the GLFWwindow can display the
    // P3 color gamut, allowing for 10-bit framebuffer images.
    bool canDisplayP3(GLFWwindow* window);

    bool isWindowOccluded(GLFWwindow* window);
} // namespace krypton::rapi::mtl
