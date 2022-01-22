#include <fmt/core.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <models/fileloader.hpp>

namespace krypton::models {
    struct PrimitiveBufferValue {
        const float* data = nullptr;
        uint64_t stride = 0;
        uint64_t count = 0;
    };

    glm::mat4 getTransformMatrix(const tinygltf::Node& node) {
        /** Both a matrix and TRS values are not allowed
         * to exist at the same time according to the spec */
        if (node.matrix.size() == 16) {
            return glm::make_mat4x4(node.matrix.data());
        } else {
            auto translation = glm::vec3(0.0f);
            auto rotation = glm::quat();
            auto scale = glm::vec3(1.0f);

            if (node.translation.size() == 3)
                translation = glm::make_vec3(node.translation.data());

            if (node.rotation.size() == 4)
                rotation = glm::make_quat(node.rotation.data());

            if (node.scale.size() == 3)
                scale = glm::make_vec3(node.scale.data());

            return glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
        }
    }

    int32_t getTinyGltfTypeSizeInBytes(uint32_t type) {
        return tinygltf::GetNumComponentsInType(type) * tinygltf::GetComponentSizeInBytes(TINYGLTF_COMPONENT_TYPE_FLOAT);
    }

    void getBufferValueForAccessor(tinygltf::Model& model, int accessorIndex, int type, PrimitiveBufferValue* bv) {
        const auto& accessor = model.accessors[accessorIndex];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        bv->data = reinterpret_cast<const float*>(&(model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
        bv->stride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : getTinyGltfTypeSizeInBytes(type);
        bv->count = accessor.count;
    };

    template <typename T, typename S>
    void writeToVector(const T* src, const size_t count, std::vector<S>& out) {
        out.resize(count);
        for (size_t i = 0; i < count; ++i)
            out[i] = static_cast<T>(src[i]);
    }
} // namespace krypton::models

void krypton::models::FileLoader::loadGltfMesh(tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Node& node) {
    auto transform = krypton::models::getTransformMatrix(node);
    auto& kMesh = meshes.emplace_back();

    for (const auto& primitive : mesh.primitives) {
        auto& kPrimitive = kMesh.primitives.emplace_back();

        {
            // We require a position attribute.
            if (primitive.attributes.find("POSITION") == primitive.attributes.end())
                continue;

            // We will first load all the different buffers (possibly the same buffer at a different
            // offset) and store them as pointers.
            PrimitiveBufferValue positions = {}, normals = {}, uvs = {};
            getBufferValueForAccessor(model, primitive.attributes.find("POSITION")->second, TINYGLTF_TYPE_VEC3, &positions);

            if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
                getBufferValueForAccessor(model, primitive.attributes.find("NORMAL")->second, TINYGLTF_TYPE_VEC3, &normals);

            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                getBufferValueForAccessor(model, primitive.attributes.find("TEXCOORD_0")->second, TINYGLTF_TYPE_VEC2, &uvs);

            for (size_t i = 0; i < positions.count; ++i) {
                /* Create new vertices */
                auto& vertex = kPrimitive.vertices.emplace_back();
                auto pos = glm::make_vec3(&positions.data[i * positions.stride]);
                vertex.pos = glm::fvec4(pos.x, pos.y, pos.z, 1.0f);

                if (normals.data != nullptr) {
                    auto normal = glm::make_vec3(&normals.data[i * normals.stride]);
                    vertex.normals = glm::fvec4(normal.x, normal.y, normal.z, 1.0f);
                }

                if (uvs.data != nullptr) {
                    vertex.uv = glm::make_vec2(&uvs.data[i * uvs.stride]);
                }
            }

            // Indices are optional with glTF.
            if (primitive.indices >= 0) {
                const auto& accessor = model.accessors[primitive.indices];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];

                auto indexCount = static_cast<size_t>(accessor.count);
                const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                switch (accessor.componentType) {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        writeToVector(static_cast<const krypton::mesh::Index*>(dataPtr), indexCount, kPrimitive.indices);
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        writeToVector(static_cast<const uint16_t*>(dataPtr), indexCount, kPrimitive.indices);
                    }
                    default: {
                        fmt::print(stderr, "Index component type {} is unsupported.", accessor.componentType);
                    }
                }
            } else {
                /* As there are no indices, we'll generate them */
                kPrimitive.indices.resize(positions.count);
                for (size_t i = 0; i < positions.count; ++i) {
                    kPrimitive.indices[i] = static_cast<krypton::mesh::Index>(i);
                }
            }
        }
    }
}

void krypton::models::FileLoader::loadGltfNode(tinygltf::Model& model, const tinygltf::Node& node) {
    // TODO: Add support for node matrices.
    if (node.mesh >= 0) {
        loadGltfMesh(model, model.meshes[node.mesh], node);
    }

    for (auto& i : node.children) {
        loadGltfNode(model, model.nodes[i]);
    }
}

bool krypton::models::FileLoader::loadGltfFile(const fs::path& path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    fs::path ext = path.extension();
    bool success = false;
    if (ext.compare(".glb") == 0) {
        success = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());
    } else if (ext.compare(".gltf") == 0) {
        success = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
    }

    if (!warn.empty())
        fmt::print("tinygltf warning: {}\n", warn);
    if (!err.empty())
        fmt::print("tinygltf error: {}\n", err);
    if (!success)
        return false;

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    for (auto& node : scene.nodes) {
        loadGltfNode(model, model.nodes[node]);
    }

    /* Load textures */
    for (const auto& tex : model.textures) {
        auto& textureFile = textures.emplace_back();
        const auto& image = model.images[tex.source];
        textureFile.width = image.width;
        textureFile.height = image.height;
        textureFile.pixels.resize(image.width * image.height * image.component);
        memcpy(textureFile.pixels.data(), image.image.data(), textureFile.pixels.size());
    }

    /* Load materials */
    for (const auto& mat : model.materials) {
        auto& kMaterial = materials.emplace_back();

        {
            auto& pbrmr = mat.pbrMetallicRoughness;
            kMaterial.baseColor = glm::fvec3(
                glm::make_vec3(pbrmr.baseColorFactor.data())); /* baseColorFactor is a vector of doubles, needs to be converted. */

            if (pbrmr.baseColorTexture.index >= 0)
                kMaterial.baseTextureIndex = pbrmr.baseColorTexture.index;
            if (pbrmr.metallicRoughnessTexture.index >= 0)
                kMaterial.pbrTextureIndex = pbrmr.metallicRoughnessTexture.index;

            kMaterial.metallicFactor = static_cast<float>(pbrmr.metallicFactor);
            kMaterial.roughnessFactor = static_cast<float>(pbrmr.roughnessFactor);
        }

        {
            /* Default indices with tinygltf are -1, which is the same
             * 'invalid' value we also use. */
            kMaterial.normalTextureIndex = mat.normalTexture.index;
            kMaterial.occlusionTextureIndex = mat.occlusionTexture.index;
            kMaterial.emissiveTextureIndex = mat.emissiveTexture.index;
        }
    }

    return true;
}

bool krypton::models::FileLoader::loadFile(const fs::path& path) {
    meshes.clear();
    materials.clear();
    textures.clear();

    if (!path.has_extension()) {
        fmt::print("Given path does not point to a file: {}\n", path.string());
        return false;
    }
    fs::path ext = path.extension();
    if (ext.compare(".glb") == 0 || ext.compare(".gltf") == 0) {
        bool ret = loadGltfFile(path);
        if (ret) {
            fmt::print("Finished loading file!");
            return true;
        }
    }

    fmt::print("Failed to load file: {}\n", path.string());
    return false;
}
