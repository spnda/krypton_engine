#pragma once

#include <string>
#include <vector>

#include <assets/material.hpp>
#include <assets/primitive.hpp>

namespace krypton::assets {
    struct Mesh final {
        std::string name = {};
        std::vector<krypton::assets::Primitive> primitives;
        glm::mat4x4 transform = glm::mat4(1.0);
    };
} // namespace krypton::assets
