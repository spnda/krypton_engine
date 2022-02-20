#pragma once

#include <cstdlib>

#include <glm/glm.hpp>

// Fuck windows.h
#undef near
#undef far

namespace krypton::rapi {
    /**
     * This represents all the camera information the rendering
     * backend should need to render. This has to be provided by
     * whoever uses the rendering API, else these default values
     * will be used. Due to aligning by 16-bytes, this structure
     * is 144 bytes big.
     */
    struct alignas(16) CameraData final {
        glm::mat4 projection = glm::mat4(1.0);
        glm::mat4 view = glm::mat4(1.0);
        float near = 0.1f;
        float far = 500.0f;
    };

    static constexpr std::size_t CAMERA_DATA_SIZE = sizeof(CameraData);
} // namespace krypton::rapi
