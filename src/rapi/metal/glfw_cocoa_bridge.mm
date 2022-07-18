#define GLFW_EXPOSE_NATIVE_COCOA
#import <GLFW/glfw3.h>
#import <GLFW/glfw3native.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CAMetalLayer.hpp>

#import <rapi/metal/glfw_cocoa_bridge.hpp>

namespace krypton::rapi::mtl {
    // I don't want to write a full wrapper for NSWindow for these three lines,
    // therefore I will just leave this single Objective-C++ file here to handle
    // this basic assignment.
    void setMetalLayerOnWindow(GLFWwindow* window, CA::MetalLayer* layer) {
        NSWindow* nswindow = glfwGetCocoaWindow(window);
        nswindow.contentView.layer = (__bridge CAMetalLayer*)layer;
        nswindow.contentView.wantsLayer = TRUE;
    }

    bool canDisplayP3(GLFWwindow* window) {
        NSScreen* nativeMonitor = nil;

        auto screen = [glfwGetCocoaWindow(window) screen];
        nativeMonitor = screen == nil ? [NSScreen mainScreen] : screen;

        if (nativeMonitor == nil)
            return false;

        return [nativeMonitor canRepresentDisplayGamut:(NSDisplayGamutP3)];
    }

    bool isWindowOccluded(GLFWwindow* window) {
        return !([glfwGetCocoaWindow(window) occlusionState] & NSWindowOcclusionStateVisible);
    }
}
