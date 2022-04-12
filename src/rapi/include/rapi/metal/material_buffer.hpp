#pragma once

#include <Metal/Metal.hpp>
#include <glm/glm.hpp>

namespace krypton::rapi::metal {
    // Represents a Material for our Metal renderer. This uses an MTL::ArgumentEncoder to store
    // material information within a buffer which is bound before every draw call.
    struct MaterialBuffer final {
        MTL::Buffer* buffer = nullptr;

        glm::vec4 baseColor = glm::fvec4(1.0);
    };
} // namespace krypton::rapi::metal
