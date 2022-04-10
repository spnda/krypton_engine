#pragma once

#ifdef RAPI_WITH_VULKAN

#include <assets/vertex.hpp>

namespace krypton::rapi::vulkan {
    /**
     * Represents a single geometry inside a bigger instance. This
     * includes its material index and offsets into the vertex
     * and index buffers of the parent instance buffers.
     */
    struct GeometryDescription {
        uint64_t vertexBufferAddress = 0;
        uint64_t indexBufferAddress = 0;
        krypton::assets::Index materialIndex = 0;
    };
} // namespace krypton::rapi::vulkan

#endif // #ifdef RAPI_WITH_VULKAN
