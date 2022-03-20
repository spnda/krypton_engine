#pragma once

#include <glm/glm.hpp>

#include <assets/vertex.hpp>

namespace krypton::assets {
    struct Material final {
        glm::fvec4 baseColor = glm::fvec4(1.0f);
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        Index baseTextureIndex = -1; // this means we should use the default, empty image.
        Index normalTextureIndex = -1;
        Index occlusionTextureIndex = -1;
        Index emissiveTextureIndex = -1;
        Index pbrTextureIndex = -1;
    };
} // namespace krypton::assets
