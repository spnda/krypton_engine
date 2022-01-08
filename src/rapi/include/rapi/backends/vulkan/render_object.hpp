#pragma once

#ifdef RAPI_WITH_VULKAN

#include <memory>

#include <mesh/mesh.hpp>
#include <rapi/backends/vulkan/buffer_descriptions.hpp>

namespace carbon {
    class BottomLevelAccelerationStructure;
    class Buffer;
} // namespace carbon

namespace krypton::rapi::vulkan {
    /**
     * A renderable object backed by some Mesh.
     */
    struct RenderObject {
        std::shared_ptr<krypton::mesh::Mesh> mesh = nullptr;

        std::vector<krypton::rapi::vulkan::GeometryDescription> geometryDescriptions = {};
        std::unique_ptr<carbon::BottomLevelAccelerationStructure> blas = nullptr;

        explicit RenderObject() = default;
    };
} // namespace krypton::rapi::vulkan

#endif // #ifdef RAPI_WITH_VULKAN
