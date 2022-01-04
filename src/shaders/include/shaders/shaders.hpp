#pragma once

#include <filesystem>

#include "spirv_cross_c.h" // We're using the C API from the submodule, as the C++ API is not 100% stable

namespace krypton::shaders {
    enum class CrossCompileTarget {
        TARGET_GLSL = SPVC_BACKEND_GLSL,
        TARGET_METAL = SPVC_BACKEND_MSL,
    };

    /* Represents a shader file. */
    struct Shader {
        std::filesystem::path filePath;
        std::string content;
    };

    /** Represents a compiled shader */
    struct ShaderCompileResult {
        std::vector<uint32_t> binary;
        std::vector<uint32_t> debugBinary;
    };

    struct CrossCompileResult {
        krypton::shaders::CrossCompileTarget target;
        std::string result;
    };

    Shader readShaderFile(std::filesystem::path path);

    /** Compiles GLSL shaders to SPIR-V */ 
    [[nodiscard]] auto compileGlslShader(const std::string& shaderName, const std::string& shaderSource, shaderc_shader_kind kind) -> ShaderCompileResult;
    
    /** Can cross compile shaders from given SPIR-V to target */
    [[nodiscard]] auto crossCompile(const std::vector<uint32_t>& spirv, CrossCompileTarget target) -> krypton::shaders::CrossCompileResult;
}
