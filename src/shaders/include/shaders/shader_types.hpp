#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace fs = std::filesystem;

namespace krypton::shaders {
    enum class ShaderSourceType : uint32_t {
        GLSL,
        HLSL,
        SLANG,
        SPIRV,
        METAL,
    };

    enum class ShaderTargetType : uint32_t {
        GLSL,
        HLSL,
        SPIRV,
        METAL,
    };

    inline bool operator==(ShaderSourceType a, ShaderTargetType b) {
        switch (a) {
            case ShaderSourceType::GLSL:
                return b == ShaderTargetType::GLSL;
            case ShaderSourceType::HLSL:
                return b == ShaderTargetType::HLSL;
            case ShaderSourceType::SPIRV:
                return b == ShaderTargetType::SPIRV;
            default:
                // Other inputs can only be slang, which we can't set as a target or the target is
                // Metal, which we cannot take as an input.
                return false;
        }
    }

    // clang-format off
    enum class ShaderStage : uint16_t {
        None        = 0,
        Vertex      = 1 <<  0,
        Fragment    = 1 <<  1,
        Geometry    = 1 <<  2,
        Compute     = 1 <<  3,
        Mesh        = 1 <<  4,
        RayGen      = 1 <<  5,
        ClosestHit  = 1 <<  6,
        Miss        = 1 <<  7,
        AnyHit      = 1 <<  8,
        Intersect   = 1 <<  9,
        Callable    = 1 << 10,
    };
    // clang-format on

    inline constexpr ShaderStage operator|(ShaderStage a, ShaderStage b) {
        return static_cast<ShaderStage>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }

    inline constexpr ShaderStage operator&(ShaderStage a, ShaderStage b) {
        return static_cast<ShaderStage>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }

    enum class TargetSpirv {
        /* slang compiler doesn't specify a SPIR-V Version, we default to 1.5 */
        SPV_1_5,
        SPV_1_6
    };

    /* Represents a shader file. */
    struct ShaderFile {
        fs::path filePath;
        std::string content;
    };

    /** Represents input data for shader compilation */
    struct ShaderCompileInput {
        fs::path filePath;
        std::span<const std::byte> source;

        std::vector<std::string> entryPoints;
        std::vector<ShaderStage> shaderStages;

        ShaderSourceType sourceType;
        ShaderTargetType targetType;
    };

    /**
     * Specifies the type of output of a compilation
     * operation.
     */
    enum class CompileResultType {
        SPIRV,
        String,
    };

    /** Represents a compiled shader */
    struct ShaderCompileResult {
        /**
         * This might be SPIR-V binary or a string, determined
         * by the resultType member.
         */
        std::vector<uint8_t> resultBytes;

        /**
         * The result type of the result. Usually SPIR-V, meaning
         * that the result points to a array of uint32_t's.
         */
        CompileResultType resultType = CompileResultType::SPIRV;

        /**
         * This is the actual size of the data in bytes. This is
         * automatically adjusted to what actual type the result
         * is in.
         */
        size_t resultSize;
    };
} // namespace krypton::shaders