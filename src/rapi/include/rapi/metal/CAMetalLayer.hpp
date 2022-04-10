#pragma once

#ifdef RAPI_WITH_METAL

#include <Foundation/NSObject.hpp>
#include <QuartzCore/CADefines.hpp>
#include <QuartzCore/CAMetalDrawable.hpp>
#include <QuartzCore/CAPrivate.hpp>

namespace CG {
    // https://developer.apple.com/documentation/coregraphics/cgsize
    struct Size {
        double width;
        double height;
    };
} // namespace CG

namespace CA {
    // All of our private classes/selectors to use the Objective-C
    // API and all of its objects. These use the helper macros from the
    // CAPrivate.hpp header that comes shipped with metal-cpp.
    namespace Private {
        namespace Class {
            _CA_PRIVATE_DEF_CLS(CALayer)
            _CA_PRIVATE_DEF_CLS(CAMetalLayer)
            _CA_PRIVATE_DEF_CLS(NSResponder)
            _CA_PRIVATE_DEF_CLS(NSWindow)
        } // namespace Class

        namespace Selector {
            _CA_PRIVATE_DEF_SEL(device, "device")
            _CA_PRIVATE_DEF_SEL(nextDrawable, "nextDrawable")
            _CA_PRIVATE_DEF_SEL(setDevice, "setDevice:")
            _CA_PRIVATE_DEF_SEL(setDrawableSize, "setDrawableSize:")
            _CA_PRIVATE_DEF_SEL(setPixelFormat, "setPixelFormat:")
        } // namespace Selector
    }     // namespace Private

    /*
     * The base CA::Layer interface.
     *
     * https://developer.apple.com/documentation/quartzcore/calayer
     */
    class Layer : public NS::Referencing<Layer> {
    public:
        [[nodiscard]] static class Layer* layer();
        [[nodiscard]] class Layer* init();
    };

    /**
     * The CA::MetalLayer class used for rendering to some drawable.
     *
     * https://developer.apple.com/documentation/quartzcore/cametallayer
     */
    class MetalLayer : public NS::Referencing<MetalLayer, ::CA::Layer> {
    public:
        [[nodiscard]] static class MetalLayer* layer();
        [[nodiscard]] CA::MetalDrawable* nextDrawable();

        [[nodiscard]] MTL::Device* device();
        void setDevice(const MTL::Device* device);
        void setPixelFormat(MTL::PixelFormat format);
        void setDrawableSize(CG::Size size);
    };
} // namespace CA

inline CA::Layer* CA::Layer::layer() {
    return Object::sendMessage<Layer*>(_CA_PRIVATE_CLS(CALayer), _CA_PRIVATE_SEL(layer));
}

inline CA::Layer* CA::Layer::init() {
    return Object::init<Layer>();
}

inline CA::MetalLayer* CA::MetalLayer::layer() {
    return Object::sendMessage<MetalLayer*>(_CA_PRIVATE_CLS(CAMetalLayer), _CA_PRIVATE_SEL(layer));
}

inline CA::MetalDrawable* CA::MetalLayer::nextDrawable() {
    return Object::sendMessage<MetalDrawable*>(this, _CA_PRIVATE_SEL(nextDrawable));
}

inline MTL::Device* CA::MetalLayer::device() {
    return Object::sendMessage<MTL::Device*>(this, _CA_PRIVATE_SEL(device));
}

inline void CA::MetalLayer::setDevice(const MTL::Device* device) {
    return Object::sendMessage<void>(this, _CA_PRIVATE_SEL(setDevice), device);
}

inline void CA::MetalLayer::setPixelFormat(MTL::PixelFormat format) {
    return Object::sendMessage<void>(this, _CA_PRIVATE_SEL(setPixelFormat), format);
}

inline void CA::MetalLayer::setDrawableSize(CG::Size size) {
    return Object::sendMessage<void>(this, _CA_PRIVATE_SEL(setDrawableSize), size);
}

#endif // #ifdef RAPI_WITH_METAL
