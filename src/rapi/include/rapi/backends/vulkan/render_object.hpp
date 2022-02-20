#pragma once

#ifdef RAPI_WITH_VULKAN

#include <memory>

#include <glm/glm.hpp>

#include <mesh/mesh.hpp>
#include <rapi/backends/vulkan/buffer_descriptions.hpp>
#include <rapi/object_handles.hpp>

namespace carbon {
    struct BottomLevelAccelerationStructure;
    class Buffer;
} // namespace carbon

namespace krypton::rapi::vulkan {
    /**
     * A renderable object backed by some Mesh.
     */
    struct RenderObject final {
        struct RenderObjectPrimitive {
            krypton::mesh::Primitive primitive;
            krypton::rapi::MaterialHandle material;
        };

        std::string name;
        std::vector<RenderObjectPrimitive> primitives;
        glm::mat4x3 transform = glm::mat4x3(1.0f);

        std::vector<krypton::rapi::vulkan::GeometryDescription> geometryDescriptions = {};
        std::unique_ptr<carbon::BottomLevelAccelerationStructure> blas;

        explicit RenderObject() = default;
    };
} // namespace krypton::rapi::vulkan

#endif // #ifdef RAPI_WITH_VULKAN
