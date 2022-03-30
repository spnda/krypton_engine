#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <stb_image.h>
// #include <tiny_gltf.h> // fileloader.hpp already includes tiny_gltf...

#include <Tracy.hpp>

#include <assets/loader/fileloader.hpp>
#include <assets/loader/ktx_loader.hpp>
#include <util/logging.hpp>

namespace krypton::assets::loader {
    struct PrimitiveBufferValue {
        const float* data = nullptr;
        uint64_t stride = 0;
        uint64_t count = 0;
    };

    struct ImageLoadData {
        enum class Format {
            // This represents that the data format is just raw pixel data and essentially consists
            // of a long vector<byte> where each 4 bytes represent a single RGBA pixel.
            RAW_RGBA,
            RAW_RGB,
        } format;
        uint32_t vkFormat;
    };

    glm::mat4 getTransformMatrix(const tinygltf::Node& node, glm::mat4x4& base) {
        /** Both a matrix and TRS values are not allowed
         * to exist at the same time according to the spec */
        if (node.matrix.size() == 16) {
            return base * glm::mat4x4(glm::make_mat4x4(node.matrix.data()));
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

            return base * glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
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
    }

    template <typename T, typename S>
    void writeToVector(const T* src, const size_t count, std::vector<S>& out) {
        ZoneScoped;
        out.resize(count);
        for (size_t i = 0; i < count; ++i)
            out[i] = static_cast<T>(src[i]);
    }
} // namespace krypton::assets::loader

namespace ka = krypton::assets;

// This is a texture data loader for tinygltf.
bool ka::loader::FileLoader::loadImageData(tinygltf::Image* image, const int imageIndex, std::string* err, std::string* warn,
                                           int requestedWidth, int requestedHeight, const unsigned char* bytes, int size, void* userData) {
    auto* imageLoadData = reinterpret_cast<ImageLoadData*>(userData);

    fs::path extension;
    if (!image->mimeType.empty()) {
        extension = tinygltf::MimeToExt(image->mimeType);
        if (extension.empty()) {
            extension = image->mimeType.substr(std::strlen("image/"));
        }
    } else {
        auto path = fs::path { image->uri };
        extension = path.extension().string().substr(1); // Get rid of the dot in the extension
    }

    if (extension == "png" || extension == "jpg" || extension == "jpeg" || extension == "bmp") {
        // We load PNG and JPEG images through STB_image
        int w = 0, h = 0, channels = 0, requiredChannels = 0;
        uint8_t* data = nullptr;

        requiredChannels = 4;
        int bits = 8;
        int pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;

        if (stbi_is_16_bit_from_memory(bytes, size)) {
            data = reinterpret_cast<uint8_t*>(stbi_load_16_from_memory(bytes, size, &w, &h, &channels, requiredChannels));
            if (data) {
                bits = 16;
                pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
            }
        }

        if (!data) {
            data = stbi_load_from_memory(bytes, size, &w, &h, &channels, requiredChannels);
        }

        if (!data) {
            if (err) {
                (*err) += fmt::format("Failed to read from image using STB. {} at URI {}", image->name, image->uri);
            }
            return false;
        }

        if (requiredChannels != 0) {
            channels = requiredChannels;
        }

        imageLoadData->format = channels == 4 ? ImageLoadData::Format::RAW_RGBA : ImageLoadData::Format::RAW_RGB;
        imageLoadData->vkFormat = channels == 4 ? 37  // VK_FORMAT_R8G8B8A8_UNORM
                                                : 23; // VK_FORMAT_R8G8B8_UNORM

        image->width = w;
        image->height = h;
        image->component = channels;
        image->bits = bits;
        image->pixel_type = pixel_type;
        image->image.resize(static_cast<size_t>(w * h * channels) * size_t(bits / 8));
        std::copy_n(data, w * h * channels * (bits / 8), image->image.begin());

        stbi_image_free(data);
        return true;
    } else if (extension == "ktx") {
        const auto* srcBytes = reinterpret_cast<const std::byte*>(bytes);
        auto bytesSize = static_cast<std::size_t>(size);

        KTX::Header header = {};
        auto levels = parseKTXHeader({ srcBytes, bytesSize }, &header);

        imageLoadData->format = ImageLoadData::Format::RAW_RGBA;
        imageLoadData->vkFormat = header.vkFormat;

        auto mainLevel = getMipmapTexture({ srcBytes, bytesSize }, &header, &levels[0]);
        image->image.resize(mainLevel.size_bytes());
        std::memcpy(image->image.data(), (void*)mainLevel.data(), mainLevel.size_bytes());

        return true;
    }

    return false;
}

void ka::loader::FileLoader::loadGltfMesh(tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Node& node,
                                          glm::mat4 parentMatrix) {
    ZoneScoped;
    auto& kMesh = meshes.emplace_back(std::make_shared<ka::Mesh>());
    kMesh->name = mesh.name;
    kMesh->transform = parentMatrix;

    for (const auto& primitive : mesh.primitives) {
        auto& kPrimitive = kMesh->primitives.emplace_back();

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
                        writeToVector(static_cast<const ka::Index*>(dataPtr), indexCount, kPrimitive.indices);
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        writeToVector(static_cast<const uint16_t*>(dataPtr), indexCount, kPrimitive.indices);
                        break;
                    }
                    default: {
                        krypton::log::err("Index component type {} is unsupported.", accessor.componentType);
                    }
                }
            } else {
                /* As there are no indices, we'll generate them */
                kPrimitive.indices.resize(positions.count);
                for (size_t i = 0; i < positions.count; ++i) {
                    kPrimitive.indices[i] = static_cast<ka::Index>(i);
                }
            }

            kPrimitive.materialIndex = primitive.material;
        }
    }
}

void ka::loader::FileLoader::loadGltfNode(tinygltf::Model& model, uint32_t nodeIndex, glm::mat4 matrix) {
    ZoneScoped;
    matrix = getTransformMatrix(model.nodes[nodeIndex], matrix);

    auto& node = model.nodes[nodeIndex];
    if (node.mesh >= 0) {
        loadGltfMesh(model, model.meshes[node.mesh], node, matrix);
    }

    for (auto& i : node.children) {
        loadGltfNode(model, i, matrix);
    }
}

bool ka::loader::FileLoader::loadGltfFile(const fs::path& path) {
    ZoneScoped;
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    ImageLoadData imageLoadData = {};
    loader.SetImageLoader(&loadImageData, &imageLoadData);

    fs::path ext = path.extension();
    bool success = false;
    if (ext.compare(".glb") == 0) {
        success = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());
    } else if (ext.compare(".gltf") == 0) {
        success = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
    }

    if (!warn.empty())
        krypton::log::warn("tinygltf warning: {}", warn);
    if (!err.empty())
        krypton::log::err("tinygltf error: {}", err);
    if (!success)
        return false;

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    for (auto& node : scene.nodes) {
        loadGltfNode(model, static_cast<uint32_t>(node), glm::mat4(1.0f));
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

            // baseColorFactor is a vector of doubles, needs to be converted.
            if (pbrmr.baseColorFactor.size() == 4) {
                kMaterial.baseColor = glm::make_vec4(pbrmr.baseColorFactor.data());
            } else {
                auto baseColor = glm::make_vec3(pbrmr.baseColorFactor.data());
                kMaterial.baseColor = glm::fvec4(baseColor, 1.0);
            }

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

bool ka::loader::FileLoader::loadFile(const fs::path& path) {
    ZoneScoped;
    meshes.clear();
    materials.clear();
    textures.clear();

    if (!path.has_filename()) {
        krypton::log::err("Given path does not point to a file: {}", path.string());
        return false;
    }

    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        krypton::log::err("Given path does not exist: {}", path.string());
        return false;
    }

    fs::path ext = path.extension();
    if (ext.compare(".glb") == 0 || ext.compare(".gltf") == 0) {
        bool ret = loadGltfFile(path);
        if (ret) {
            krypton::log::log("Finished loading model file {}", path.string());
            return true;
        }
    }

    krypton::log::err("Failed to load file: {}", path.string());
    return false;
}
