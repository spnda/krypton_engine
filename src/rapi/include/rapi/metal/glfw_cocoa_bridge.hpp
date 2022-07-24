#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace krypton::rapi::mtl {
    // Returns 'true' if the display that is currently displaying the GLFWwindow can display the
    // P3 color gamut, allowing for 10-bit framebuffer images.
    bool canDisplayP3(GLFWwindow* window);
} // namespace krypton::rapi::mtl
