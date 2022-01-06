#pragma once

#define TINYGLTF_IMPLEMENTATION

#include <filesystem>

#include <glm/glm.hpp>
#include <tiny_gltf.h>

#include <mesh/material.hpp>
#include <mesh/mesh.hpp>
#include <mesh/texture.hpp>

namespace fs = std::filesystem;

namespace krypton::models {
    class FileLoader {
        std::vector<krypton::mesh::Mesh> meshes;
        std::vector<krypton::mesh::Material> materials;
        std::vector<krypton::mesh::Texture> textures;

        void loadGltfMesh(tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Node& node);
        void loadGltfNode(tinygltf::Model& model, const tinygltf::Node& node);
        [[nodiscard]] bool loadGltfFile(const fs::path& path);

    public:
        [[nodiscard]] bool loadFile(const fs::path& path);
    };
}
