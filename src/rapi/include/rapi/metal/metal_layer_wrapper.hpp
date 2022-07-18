#pragma once

#include <CoreGraphics/CGGeometry.h>
#include <QuartzCore/CAMetalLayer.hpp>

namespace CA {
    namespace Private::Selector {
        _CA_PRIVATE_DEF_SEL(maximumDrawableCount, "maximumDrawableCount");
        _CA_PRIVATE_DEF_SEL(setMaximumDrawableCount_, "setMaximumDrawableCount:");
        _CA_PRIVATE_DEF_SEL(setContentsScale_, "setContentsScale:");
    } // namespace Private::Selector

    /**
     * Wraps CA::MetalLayer from metal-cpp, adds setContentScale, and a few other convenience
     * methods which are not wrapped by metal-cpp itself.
     */
    class MetalLayerWrapper final : public CA::MetalLayer {
    public:
        auto maximumDrawableCount() -> NS::UInteger;
        // You can set this value to 2 or 3 only; if you pass a different value, Core Animation
        // ignores the value and throws an exception.
        void setMaximumDrawableCount(NS::UInteger count);

        void setContentsScale(CGFloat scale);
    };
} // namespace CA

_CA_INLINE NS::UInteger CA::MetalLayerWrapper::maximumDrawableCount() {
    return Object::sendMessage<NS::UInteger>(this, _CA_PRIVATE_SEL(maximumDrawableCount));
}

_CA_INLINE void CA::MetalLayerWrapper::setMaximumDrawableCount(NS::UInteger count) {
    return Object::sendMessage<void>(this, _CA_PRIVATE_SEL(setMaximumDrawableCount_), count);
}

_CA_INLINE void CA::MetalLayerWrapper::setContentsScale(CGFloat scale) {
    return Object::sendMessage<void>(this, _CA_PRIVATE_SEL(setContentsScale_), scale);
}
