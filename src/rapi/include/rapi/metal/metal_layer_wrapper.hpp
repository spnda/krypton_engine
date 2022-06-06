#pragma once

#ifdef RAPI_WITH_METAL

#include <CoreGraphics/CGGeometry.h>
#include <QuartzCore/CAMetalLayer.hpp>

namespace CA {
    namespace Private::Selector {
        _CA_PRIVATE_DEF_SEL(setContentsScale, "setContentsScale:");
    } // namespace Private::Selector

    /**
     * Wraps CA::MetalLayer from metal-cpp and adds setContentScale.
     */
    class MetalLayerWrapper : public CA::MetalLayer {
    public:
        // metal-cpp does not expose this function, even though it is pretty important.
        void setContentsScale(CGFloat scale);
    };
} // namespace CA

inline void CA::MetalLayerWrapper::setContentsScale(CGFloat scale) {
    return Object::sendMessage<void>(this, _CA_PRIVATE_SEL(setContentsScale), scale);
}

#endif // #ifdef RAPI_WITH_METAL
