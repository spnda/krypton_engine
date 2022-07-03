#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include <assets/material.hpp>
#include <assets/mesh.hpp>
#include <assets/texture.hpp>

namespace fs = std::filesystem;

namespace tinygltf {
    class Node;
    class Mesh;
    class Model;
} // namespace tinygltf

namespace krypton::assets::loader {
    class FileLoader final {
        void loadGltfMesh(tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Node& node, glm::mat4 parentMatrix);
        void loadGltfNode(tinygltf::Model& model, uint32_t nodeIndex, glm::mat4 matrix);
        [[nodiscard]] bool loadGltfFile(const fs::path& path);

    public:
        std::vector<std::shared_ptr<krypton::assets::Mesh>> meshes;
        std::vector<krypton::assets::Material> materials;
        std::vector<krypton::assets::Texture> textures;

        explicit FileLoader() = default;

        [[nodiscard]] bool loadFile(const fs::path& path);
    };
} // namespace krypton::assets::loader
