#pragma once

#ifdef RAPI_WITH_METAL

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <rapi/metal/CAMetalLayer.hpp>

namespace krypton::rapi::metal {
    void setMetalLayerOnWindow(GLFWwindow* window, CA::MetalLayer* layer);
}

#endif // #ifdef RAPI_WITH_METAL
