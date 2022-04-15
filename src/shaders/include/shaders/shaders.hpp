#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

#ifdef WITH_SLANG_SHADERS
#include <slang.h>
#endif

#ifdef WITH_GLSLANG_SHADERS
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Include/glslang_c_shader_types.h>
#endif

#include <spirv_cross_c.h> // We're using the C API from the submodule, as the C++ API is not 100% stable

namespace fs = std::filesystem;

namespace krypton::shaders {
    enum class ShaderSourceType : int {
        GLSL,
        HLSL,
        SLANG,
        SPIRV,
    };

    enum class ShaderTargetType {
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

    enum class ShaderStage {
        Vertex,
        Fragment,
        Geometry,
        Compute,
        Mesh,
        RayGen,
        ClosestHit,
        Miss,
        AnyHit,
        Intersect,
        Callable,
    };

    enum class TargetSpirv {
        /* slang compiler doesn't specify a SPIR-V Version, we default to 1.5 */
        SPV_1_5,
        SPV_1_6
    };

    /* Represents a shader file. */
    struct ShaderFile {
        std::filesystem::path filePath;
        std::string content;
    };

    /** Represents input data for shader compilation */
    struct ShaderCompileInput {
        fs::path filePath;
        std::vector<uint8_t> source;

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

    /**
     * Reads given file into a Shader object which effectively
     * just consists of the filesystem path and a string
     * of its contents.
     */
    [[nodiscard]] ShaderFile readShaderFile(const fs::path& path);

    [[nodiscard]] std::vector<uint8_t> readBinaryShaderFile(const fs::path& path);

    /**
     * Compiles a shader from [sourceType] to [targetType]. May use
     * SPIRV-Cross if the types cannot be directly converted.
     * This also automatically reads the file contents depending
     * on [sourceType].
     */
    [[nodiscard]] auto compileShaders(const fs::path& shaderFileName, ShaderStage stage, ShaderSourceType sourceType,
                                      ShaderTargetType targetType) noexcept(false) -> std::vector<ShaderCompileResult>;

    /**
     * Compiles all inputs and if all compilation steps should succeed,
     * a vector of the same size with the results is returned. Errors and
     * warnings are automatically printed into the console.
     */
    [[nodiscard]] auto compileShaders(const std::vector<ShaderCompileInput>& shaderInputs) noexcept(false)
        -> std::vector<ShaderCompileResult>;
} // namespace krypton::shaders
