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

    uint32_t getScreenPixelFormat(GLFWwindow* window, bool srgb) {
        auto* monitor = glfwGetWindowMonitor(window);

        NSScreen* nativeMonitor = nil;
        if (monitor == nil) {
            // We're not in fullscreen. Instead, we'll query the display gamut of the "main"
            // monitor through [NSScreen main].
            nativeMonitor = [NSScreen mainScreen];
        } else {
            // If we're fullscreen we can actually fetch the proper NSScreen for our window.
            CGDirectDisplayID cocoaMonitor = glfwGetCocoaMonitor(monitor);
            const auto unitNumber = CGDisplayUnitNumber(cocoaMonitor);

            for (NSScreen* screen in [NSScreen screens]) {
                NSNumber* screenNumber = [screen deviceDescription][@"NSScreenNumber"];

                if (CGDisplayUnitNumber([screenNumber unsignedIntValue]) == unitNumber) {
                    nativeMonitor = screen;
                    break;
                }
            }
        }

        if (nativeMonitor == nil)
            return srgb ? MTLPixelFormatBGRA8Unorm_sRGB : MTLPixelFormatBGRA8Unorm;

        bool canDisplayP3 = [nativeMonitor canRepresentDisplayGamut:(NSDisplayGamutP3)];
        return canDisplayP3 ? (srgb ? MTLPixelFormatBGRA10_XR_sRGB : MTLPixelFormatBGRA10_XR)
                            : (srgb ? MTLPixelFormatBGRA8Unorm_sRGB : MTLPixelFormatBGRA8Unorm);
    }
}

#endif // #ifdef RAPI_WITH_METAL
