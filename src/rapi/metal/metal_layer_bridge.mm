#ifdef RAPI_WITH_METAL

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <rapi/metal/CAMetalLayer.hpp>
#include <rapi/metal/metal_layer_bridge.hpp>

namespace krypton::rapi::metal {
    // I don't want to write a full wrapper for NSWindow for these three lines,
    // therefore I will just leave this single Objective-C++ file here to handle
    // this basic assignment.
    void setMetalLayerOnWindow(GLFWwindow* window, CA::MetalLayer* layer) {
        NSWindow* nswindow = glfwGetCocoaWindow(window);
        nswindow.contentView.layer = (__bridge CALayer*)layer;
        nswindow.contentView.wantsLayer = YES;
    }
}

#endif // #ifdef RAPI_WITH_METAL
