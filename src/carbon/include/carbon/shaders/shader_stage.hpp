#pragma once

#include <carbon/vulkan.hpp>

namespace carbon {
    enum class ShaderStage : uint64_t {
        Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
        Vertex = VK_SHADER_STAGE_VERTEX_BIT,
        RayGeneration = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        ClosestHit = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        RayMiss = VK_SHADER_STAGE_MISS_BIT_KHR,
        AnyHit = VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
        Intersection = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
        Callable = VK_SHADER_STAGE_CALLABLE_BIT_KHR,
    };
}
