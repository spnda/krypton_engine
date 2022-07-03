#pragma once

#include <shaders/shader_types.hpp>

namespace krypton::shaders {
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
