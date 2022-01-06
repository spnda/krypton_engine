#pragma once

#include <glm/glm.hpp>

#include <mesh/vertex.hpp>

namespace krypton::mesh {
    struct Material final {
        glm::vec3 baseColor = glm::vec3(1.0f);
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        Index baseTextureIndex = -1; // this means we should use the default, empty image.
        Index normalTextureIndex = -1;
        Index occlusionTextureIndex = -1;
        Index emissiveTextureIndex = -1;
        Index pbrTextureIndex = -1;
    };
}
