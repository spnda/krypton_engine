#pragma once

#ifdef RAPI_WITH_METAL

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <rapi/metal/CAMetalLayer.hpp>

namespace krypton::rapi::metal {
    void setMetalLayerOnWindow(GLFWwindow* window, CA::MetalLayer* layer);

    // Gets the MTLPixelFormat that is the best suitable for the screen that is currently
    // displaying the window.
    uint32_t getScreenPixelFormat(GLFWwindow* window, bool srgb = true);
} // namespace krypton::rapi::metal

#endif // #ifdef RAPI_WITH_METAL
