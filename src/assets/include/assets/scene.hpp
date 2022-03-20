#pragma once

#include <string>
#include <vector>

#include <assets/mesh.hpp>

namespace krypton::assets {
    struct Scene final {
        std::string name = {};
        std::vector<krypton::assets::Mesh> meshes;
    };
} // namespace krypton::assets
