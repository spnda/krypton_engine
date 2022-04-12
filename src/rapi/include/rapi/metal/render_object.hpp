#pragma once

#include <Metal/MTLBuffer.hpp>

#include <assets/primitive.hpp>
#include <util/handle.hpp>

namespace krypton::rapi::metal {
    struct Primitive final {
        krypton::assets::Primitive primitive = {};
        krypton::util::Handle<"Material"> material;
        size_t indexBufferOffset = 0;
        size_t indexCount = 0;
        int64_t baseVertex = 0;
    };

    struct RenderObject final {
        std::vector<Primitive> primitives = {};
        glm::mat4x3 transform = glm::mat4x3(1.0f);

        MTL::Buffer* vertexBuffer;
        MTL::Buffer* indexBuffer;
        MTL::Buffer* instanceBuffer;
        uint64_t totalVertexCount = 0, totalIndexCount = 0;
        std::vector<uint64_t> bufferVertexOffsets;
        std::vector<uint64_t> bufferIndexOffsets;
    };
} // namespace krypton::rapi::metal
