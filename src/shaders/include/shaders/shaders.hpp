#pragma once

#include <filesystem>
#include <vector>

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Include/glslang_c_shader_types.h>

#include <spirv_cross_c.h> // We're using the C API from the submodule, as the C++ API is not 100% stable

namespace krypton::shaders {
    enum class CrossCompileTarget {
        TARGET_GLSL = SPVC_BACKEND_GLSL,
        TARGET_METAL = SPVC_BACKEND_MSL,
    };

    enum class ShaderSourceType {
        SOURCE_GLSL = GLSLANG_SOURCE_GLSL,
        SOURCE_HLSL = GLSLANG_SOURCE_HLSL,
    };

    enum class ShaderStage {
        Vertex = GLSLANG_STAGE_VERTEX,
        Fragment = GLSLANG_STAGE_FRAGMENT,
        RayGen = GLSLANG_STAGE_RAYGEN_NV,
        ClosestHit = GLSLANG_STAGE_CLOSESTHIT_NV,
        AnyHit = GLSLANG_STAGE_ANYHIT_NV,
        Intersect = GLSLANG_STAGE_INTERSECT_NV,
    };

    enum class TargetSpirv {
        SPV_1_5 = GLSLANG_TARGET_SPV_1_5,
        SPV_1_6 = GLSLANG_TARGET_SPV_1_6
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
    [[nodiscard]] auto compileGlslShader(const std::string& shaderName, const std::string& shaderSource, ShaderStage shaderStage, TargetSpirv target) -> ShaderCompileResult;
    
    /** Can cross compile shaders from given SPIR-V to target */
    [[nodiscard]] auto crossCompile(const std::vector<uint32_t>& spirv, CrossCompileTarget target) -> krypton::shaders::CrossCompileResult;
}
