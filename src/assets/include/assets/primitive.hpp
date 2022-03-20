#pragma once

#include <vector>

#include <glm/glm.hpp>

#include <assets/vertex.hpp>

namespace krypton::assets {
    struct Primitive final {
        std::vector<krypton::assets::Vertex> vertices = {};
        std::vector<krypton::assets::Index> indices = {};
        Index materialIndex = 0;
    };
} // namespace krypton::assets
