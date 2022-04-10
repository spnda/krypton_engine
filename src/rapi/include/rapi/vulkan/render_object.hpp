#pragma once

#ifdef RAPI_WITH_VULKAN

#include <memory>

#include <glm/glm.hpp>

#include <assets/mesh.hpp>
#include <rapi/vulkan/buffer_descriptions.hpp>
#include <util/handle.hpp>

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
            krypton::assets::Primitive primitive;
            krypton::util::Handle<"Material"> material;
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
