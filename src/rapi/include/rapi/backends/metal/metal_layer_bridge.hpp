#pragma once

#include <GLFW/glfw3.h>

#include <rapi/backends/metal/CAMetalLayer.hpp>

namespace krypton::rapi::metal {
    void setMetalLayerOnWindow(GLFWwindow* window, CA::MetalLayer* layer);
}
