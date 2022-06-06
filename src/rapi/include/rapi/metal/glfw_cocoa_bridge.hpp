#pragma once

#ifdef RAPI_WITH_METAL

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <Metal/MTLPixelFormat.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

namespace krypton::rapi::metal {
    void setMetalLayerOnWindow(GLFWwindow* window, CA::MetalLayer* layer);

    // Gets the MTLPixelFormat that is the best suitable for the screen that is currently
    // displaying the window.
    MTL::PixelFormat getScreenPixelFormat(GLFWwindow* window, bool srgb = true);

    bool isWindowOccluded(GLFWwindow* window);
} // namespace krypton::rapi::metal

#endif // #ifdef RAPI_WITH_METAL
