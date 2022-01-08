#pragma once

#include <string>
#include <vector>

#include <mesh/mesh.hpp>

namespace krypton::mesh {
    struct Scene final {
        std::string name = {};
        std::vector<krypton::mesh::Mesh> meshes;
    };
} // namespace krypton::mesh
