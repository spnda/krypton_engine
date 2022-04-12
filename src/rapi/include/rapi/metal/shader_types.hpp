#pragma once

// We should be using simd types for Metal shaders, but it's unnecessary to always convert
// glm types to simd types...
#include <glm/glm.hpp>
#include <simd/simd.h>

namespace krypton::rapi::metal {
    struct InstanceData {
        glm::mat4x4 instanceTransform = glm::mat4x4(1.0f);
    };
} // namespace krypton::rapi::metal
