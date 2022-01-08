#pragma once

#include <string>
#include <vector>

#include <mesh/material.hpp>
#include <mesh/primitive.hpp>

namespace krypton::mesh {
    struct Mesh final {
        std::string name = {};
        std::vector<krypton::mesh::Primitive> primitives;
        glm::mat4x4 transform = glm::mat4(1.0);
    };
} // namespace krypton::mesh
