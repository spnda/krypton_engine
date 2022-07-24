// We can't use ObjC's __bridge with the new C++ casts.
#pragma clang diagnostic ignored "-Wold-style-cast"

#define GLFW_EXPOSE_NATIVE_COCOA
#import <GLFW/glfw3.h>
#import <GLFW/glfw3native.h>
#import <Tracy.hpp>
#import <glm/vec2.hpp>

#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CAMetalLayer.hpp>

#import <Metal/MTLDevice.hpp>
#import <rapi/metal/metal_layer_wrapper.hpp>
#import <rapi/window.hpp>

namespace kr = krypton::rapi;

bool kr::Window::isOccluded() const {
    ZoneScoped;
    return !([glfwGetCocoaWindow(window) occlusionState] & NSWindowOcclusionStateVisible);
}

CA::MetalLayer* kr::Window::createMetalLayer(MTL::Device* device, MTL::PixelFormat pixelFormat) const {
    ZoneScoped;
    auto layer = reinterpret_cast<CA::MetalLayerWrapper*>(CA::MetalLayer::layer());
    if (device != nullptr)
        layer->setDevice(device);
    if (pixelFormat != MTL::PixelFormatInvalid)
        layer->setPixelFormat(pixelFormat);
    layer->setContentsScale(static_cast<CGFloat>(getContentScale().x));

    // I don't want to write a full wrapper for NSWindow for these three lines.
    NSWindow* nswindow = glfwGetCocoaWindow(window);
    nswindow.contentView.layer = (__bridge CAMetalLayer*)layer; // NOLINT
    nswindow.contentView.wantsLayer = TRUE;

    return layer;
}
