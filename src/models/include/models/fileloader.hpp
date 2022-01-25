#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <tiny_gltf.h>

#include <mesh/material.hpp>
#include <mesh/mesh.hpp>
#include <mesh/texture.hpp>

namespace fs = std::filesystem;

namespace krypton::models {
    class FileLoader final {
        void loadGltfMesh(tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Node& node, glm::mat4 parentMatrix);
        void loadGltfNode(tinygltf::Model& model, uint32_t nodeIndex, glm::mat4 matrix);
        [[nodiscard]] bool loadGltfFile(const fs::path& path);

    public:
        std::vector<std::shared_ptr<krypton::mesh::Mesh>> meshes;
        std::vector<krypton::mesh::Material> materials;
        std::vector<krypton::mesh::Texture> textures;

        explicit FileLoader() = default;

        [[nodiscard]] bool loadFile(const fs::path& path);
    };
} // namespace krypton::models
