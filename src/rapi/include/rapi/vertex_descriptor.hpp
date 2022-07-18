#pragma once

#include <cstdint>

#include <vector>

namespace krypton::rapi {
    enum class VertexFormat : uint32_t {
        // Four 32-bit signed floats.
        RGBA32_FLOAT,
        // Three 32-bit signed floats.
        RGB32_FLOAT,
        // Two 32-bit signed floats.
        RG32_FLOAT,
        // Four 8-bit unsigned normalized integers.
        RGBA8_UNORM,
    };

    enum class VertexInputRate : uint8_t {
        Vertex = 0,
        Instance = 1,
    };

    struct VertexBufferDescriptor {
        uint32_t stride;
        VertexInputRate inputRate = VertexInputRate::Vertex;
    };

    struct VertexAttributeDescriptor {
        // The offset from the beginning of the buffer referenced in [bufferIndex] in bytes.
        uint32_t offset;
        // The VertexBufferDescriptor to reference.
        uint32_t bufferIndex;
        // The format of this specific attribute
        VertexFormat format;
    };

    struct VertexDescriptor final {
        // Note that the index within this vector is relevant!
        std::vector<VertexBufferDescriptor> buffers;
        std::vector<VertexAttributeDescriptor> attributes;
    };
} // namespace krypton::rapi
