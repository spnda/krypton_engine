#pragma once

#include <glm/glm.hpp>

namespace krypton::mesh {
    /* Index has to be signed as we use negative values
     * to represent missing/no texture. */
    using Index = int32_t;

    struct Vertex final {
        glm::fvec4 pos;
        glm::fvec4 color = glm::fvec4(1.0);
        glm::fvec4 normals;
        glm::fvec2 uv;
        glm::fvec2 padding;
    };

    static constexpr uint32_t VERTEX_STRIDE = sizeof(Vertex);
} // namespace krypton::mesh
