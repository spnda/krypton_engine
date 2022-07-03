#ifdef RAPI_WITH_METAL

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

    MTL::PixelFormat getScreenPixelFormat(GLFWwindow* window, bool srgb) {
        NSScreen* nativeMonitor = nil;

        NSWindow* nsWindow = glfwGetCocoaWindow(window);
        if ([nsWindow respondsToSelector:@selector(screen)]) {
            auto screen = [nsWindow screen];
            nativeMonitor = screen == nil ? [NSScreen mainScreen] : screen;
        } else {
            auto* monitor = glfwGetWindowMonitor(window);

            // We're not in fullscreen. Instead, we'll query the display gamut of the "main"
            // monitor through [NSScreen main].
            if (monitor == nil)
                nativeMonitor = [NSScreen mainScreen];

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
            return srgb ? MTL::PixelFormatBGRA8Unorm_sRGB : MTL::PixelFormatBGRA8Unorm;

        bool canDisplayP3 = [nativeMonitor canRepresentDisplayGamut:(NSDisplayGamutP3)];
        return canDisplayP3 ? (srgb ? MTL::PixelFormatBGRA10_XR_sRGB : MTL::PixelFormatBGRA10_XR)
                            : (srgb ? MTL::PixelFormatBGRA8Unorm_sRGB : MTL::PixelFormatBGRA8Unorm);
    }

    bool isWindowOccluded(GLFWwindow* window) {
        return !([glfwGetCocoaWindow(window) occlusionState] & NSWindowOcclusionStateVisible);
    }
}

#endif // #ifdef RAPI_WITH_METAL
