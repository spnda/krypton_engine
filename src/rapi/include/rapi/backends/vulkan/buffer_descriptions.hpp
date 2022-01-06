#pragma once

#ifdef RAPI_WITH_VULKAN

#include <mesh/vertex.hpp>

namespace krypton::rapi::vulkan {
    /**
     * Struct referencing buffer addresses for various mesh related buffers.
     * Used for the shader to obtain vertex and material data per primitive.
     */
    struct InstanceDescription {
        VkDeviceAddress vertexBufferAddress = 0;
        VkDeviceAddress indexBufferAddress = 0;
        VkDeviceAddress geometryBufferAddress = 0;
    };

    /**
     * Represents a single geometry inside a bigger instance. This
     * includes its material index and offsets into the vertex
     * and index buffers of the parent instance buffers.
     */
    struct GeometryDescription {
        uint64_t meshBufferVertexOffset = 0;
        uint64_t meshBufferIndexOffset = 0;
        krypton::mesh::Index materialIndex = 0;
    };
}

#endif // #ifdef RAPI_WITH_VULKAN
