#define GLFW_EXPOSE_NATIVE_COCOA
#import <GLFW/glfw3.h>
#import <GLFW/glfw3native.h>

#import <rapi/metal/glfw_cocoa_bridge.hpp>

namespace krypton::rapi::mtl {
    bool canDisplayP3(GLFWwindow* window) {
        NSWindow* nsWindow = glfwGetCocoaWindow(window);
        if (![nsWindow respondsToSelector:@selector(screen)])
            return false;

        auto screen = [nsWindow screen];
        NSScreen* nativeMonitor = screen == nil ? [NSScreen mainScreen] : screen;

        if (nativeMonitor == nil)
            return false;

        return [nativeMonitor canRepresentDisplayGamut:(NSDisplayGamutP3)];
    }
}
