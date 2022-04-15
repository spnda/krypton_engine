#include <fstream>
#include <iostream>
#include <span>
#include <sstream>
#include <string>

#ifdef WITH_SLANG_SHADERS
#include <slang.h>
#endif

#ifdef WITH_GLSLANG_SHADERS
#include <glslang/Include/ResourceLimits.h>
#include <shaders/glslang_resource.hpp>
#endif

#include <util/assert.hpp>

#include <shaders/shaders.hpp>
#include <util/logging.hpp>

namespace krypton::shaders {
    std::vector<krypton::shaders::ShaderCompileResult> compileSingleShader(const ShaderCompileInput& shaderInput);

#ifdef WITH_GLSLANG_SHADERS
    ShaderCompileResult glslangCompileShader(const krypton::shaders::ShaderCompileInput& shaderInput);
#endif

    /**
     * Uses SPIRV-Cross to compile SPIR-V assembly into another shader type.
     * Mainly used for converting SPIR-V to Metal Shading Language.
     */
    ShaderCompileResult spirvCrossCompile(std::span<const uint32_t> spirv, krypton::shaders::ShaderTargetType target);

#ifdef WITH_SLANG_SHADERS
    std::vector<ShaderCompileResult> slangCompileShader(const krypton::shaders::ShaderCompileInput& shaderInput);
#endif
} // namespace krypton::shaders

krypton::shaders::ShaderFile krypton::shaders::readShaderFile(const fs::path& path) {
    std::ifstream is(path, std::ios::binary);

    if (is.is_open()) {
        std::stringstream ss;
        ss << is.rdbuf();
        return { path, ss.str() };
    } else {
        krypton::log::throwError("Failed to open shader file: {}", path.string());
    }
}

std::vector<uint8_t> krypton::shaders::readBinaryShaderFile(const fs::path& path) {
    std::ifstream is(path, std::ios::binary);

    if (is.is_open()) {
        return { std::istreambuf_iterator<char>(is), {} };
    } else {
        krypton::log::throwError("Failed to open shader file: {}", path.string());
    }
}

std::vector<krypton::shaders::ShaderCompileResult>
krypton::shaders::compileSingleShader(const krypton::shaders::ShaderCompileInput& shaderInput) {
    if (shaderInput.sourceType == shaderInput.targetType) {
        // We don't need compilation, just return the already existing data
        // As the std::string container might get destroyed and to stick to
        // behaviour, we'll reallocate the data.
        ShaderCompileResult compileResult;
        compileResult.resultSize = shaderInput.source.size() * sizeof(uint8_t) + 1; // +1 for the null terminator character.

        auto* bytes = reinterpret_cast<const uint8_t*>(shaderInput.source.data());
        std::vector<uint8_t> newData(compileResult.resultSize);
        std::memcpy(newData.data(), bytes, compileResult.resultSize);

        compileResult.resultBytes = std::move(newData);
        return { compileResult };
    }

    if (shaderInput.sourceType == ShaderSourceType::SLANG) {
        // slang can only be compiled using the slang compiler under Windows or Linux,
        // however, it might still not be present on those platforms either.
#ifndef WITH_SLANG_SHADERS
        krypton::log::throwError("Cannot compile slang shaders when slang is not installed.");
#else
        return slangCompileShader(shaderInput);
#endif
    }

#ifdef WITH_SLANG_SHADERS
    // We're on Windows or Linux and will therefore use slang to compile slang or HLSL shaders.
    // We also cannot compile to Metal (yet, we could use Metal Developer Command Line Tools)
    // and couldn't use it anyway, so we'll leave this blank.
    if (shaderInput.sourceType != ShaderSourceType::GLSL && shaderInput.sourceType != ShaderSourceType::SPIRV &&
        shaderInput.targetType != ShaderTargetType::METAL) {
        // slang can compile SLANG, HLSL to HLSL, GLSL, and SPIR-V.
        return slangCompileShader(shaderInput);
    }
#endif

#ifdef WITH_GLSLANG_SHADERS
    // As we cannot use slang, but we use glslang and spirv-cross.
    if (shaderInput.targetType == ShaderTargetType::SPIRV) {
        // We can simply use glslang to compile the given GLSL or HLSL to SPIR-V.
        // We'll also currently default to SPIR-V 1.5, as 1.6 is not officially supported
        // by Vulkan.
        return std::vector { glslangCompileShader(shaderInput) };
    } else {
        // Use SPIRV-Cross to compile GLSL -> SPIR-V -> HLSL or HLSL -> SPIR-V -> GLSL.
        // TODO.
        krypton::log::throwError("Cross compilation with SPIRV-Cross is not yet implemented");
    }
#endif

    if (shaderInput.sourceType == ShaderSourceType::SPIRV) {
        std::span<const uint32_t> spvData = { reinterpret_cast<const uint32_t*>(shaderInput.source.data()),
                                              shaderInput.source.size() / sizeof(uint32_t) };
        return { spirvCrossCompile(spvData, shaderInput.targetType) };
    }

    krypton::log::throwError("Failed to find a way how to compile shader: {}", shaderInput.filePath.string());
    return {};
}

#ifdef WITH_GLSLANG_SHADERS
krypton::shaders::ShaderCompileResult krypton::shaders::glslangCompileShader(const krypton::shaders::ShaderCompileInput& shaderInput) {
    glslang_source_t language = GLSLANG_SOURCE_NONE;
    switch (shaderInput.sourceType) {
        case ShaderSourceType::GLSL:
            language = GLSLANG_SOURCE_GLSL;
            break;
        case ShaderSourceType::HLSL:
            language = GLSLANG_SOURCE_HLSL;
            break;
        case ShaderSourceType::SLANG: {
            krypton::log::throwError("[glslang] Cannot use slang to compile from slang");
            break;
        }
        case ShaderSourceType::SPIRV: {
            krypton::log::throwError("[glslang] Cannot use slang to compile from SPIR-V");
            break;
        }
        default: {
            krypton::log::throwError("[glslang] Unrecognized shader source type: {}", (uint32_t)shaderInput.sourceType);
        }
    }

    if (shaderInput.shaderStages.empty()) {
        krypton::log::throwError("[glslang] No shader stage was provided.");
    }

    glslang_stage_t stage;
    switch (shaderInput.shaderStages[0]) {
        case ShaderStage::Vertex:
            stage = GLSLANG_STAGE_VERTEX;
            break;
        case ShaderStage::Fragment:
            stage = GLSLANG_STAGE_FRAGMENT;
            break;
        case ShaderStage::Geometry:
            stage = GLSLANG_STAGE_GEOMETRY;
            break;
        case ShaderStage::Compute:
            stage = GLSLANG_STAGE_COMPUTE;
            break;
        case ShaderStage::Mesh:
            stage = GLSLANG_STAGE_MESH_NV;
            break;
        case ShaderStage::RayGen:
            stage = GLSLANG_STAGE_RAYGEN_NV;
            break;
        case ShaderStage::ClosestHit:
            stage = GLSLANG_STAGE_CLOSESTHIT_NV;
            break;
        case ShaderStage::Miss:
            stage = GLSLANG_STAGE_MISS_NV;
            break;
        case ShaderStage::AnyHit:
            stage = GLSLANG_STAGE_ANYHIT_NV;
            break;
        case ShaderStage::Intersect:
            stage = GLSLANG_STAGE_INTERSECT_NV;
            break;
        case ShaderStage::Callable:
            stage = GLSLANG_STAGE_CALLABLE_NV;
            break;
        default: {
            krypton::log::throwError("[glslang] Unrecognized shader stage type: {}", static_cast<uint32_t>(shaderInput.shaderStages[0]));
        }
    }

    // TODO: Make this a configurable setting.
    glslang_target_language_version_t spvVersion = GLSLANG_TARGET_SPV_1_6;

    const glslang_input_t input = {
        .language = language,
        .stage = static_cast<glslang_stage_t>(stage),
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_3,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = spvVersion,
        .code = reinterpret_cast<const char*>(shaderInput.source.data()),
        .default_version = 460,
        .default_profile = GLSLANG_NO_PROFILE,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = &krypton::shaders::DefaultTBuiltInResource,
    };

    glslang_initialize_process();

    // This automatically splits the string into lines, so that everything
    // is not printed in one print().
    auto glslangPrintError = [shaderInput](const std::string& string) {
        std::istringstream ss(string);
        std::string line;
        while (std::getline(ss, line)) {
            krypton::log::err("{}: {}", shaderInput.filePath.string(), line);
        }
    };

    glslang_shader_t* shader = glslang_shader_create(&input);
    if (!glslang_shader_preprocess(shader, &input)) {
        glslangPrintError(glslang_shader_get_info_log(shader));
        return {};
    }
    if (!glslang_shader_parse(shader, &input)) {
        glslangPrintError(glslang_shader_get_info_log(shader));
        return {};
    }
    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        glslangPrintError(glslang_shader_get_info_log(shader));
        return {};
    }

    glslang_program_SPIRV_generate(program, input.stage);
    if (glslang_program_SPIRV_get_messages(program)) {
        krypton::log::err("[glslang] {}", glslang_program_SPIRV_get_messages(program));
    }
    glslang_shader_delete(shader);

    auto* spvptr = glslang_program_SPIRV_get_ptr(program);
    size_t size = glslang_program_SPIRV_get_size(program);

    ShaderCompileResult compileResult = {};
    compileResult.resultType = CompileResultType::Spirv;
    compileResult.resultSize = size * sizeof(uint32_t);

    std::vector<uint8_t> newData(compileResult.resultSize);
    std::memcpy(newData.data(), reinterpret_cast<uint8_t*>(spvptr), compileResult.resultSize);
    compileResult.resultBytes = std::move(newData);

    return compileResult;
}
#endif

krypton::shaders::ShaderCompileResult krypton::shaders::spirvCrossCompile(std::span<const uint32_t> spirv,
                                                                          krypton::shaders::ShaderTargetType target) {
    spvc_backend backendTarget = SPVC_BACKEND_NONE;
    switch (target) {
        using namespace krypton::shaders;
        case ShaderTargetType::GLSL:
            backendTarget = SPVC_BACKEND_GLSL;
            break;
        case ShaderTargetType::HLSL:
            backendTarget = SPVC_BACKEND_HLSL;
            break;
        case ShaderTargetType::METAL: {
            backendTarget = SPVC_BACKEND_MSL;
            break;
        }
        case ShaderTargetType::SPIRV: {
            krypton::log::throwError("[spirv-cross] Cannot use SPIRV-Cross to generate SPIR-V.");
        }
        default: {
            krypton::log::throwError("[spirv-cross] Unrecognized shader target type: {}", (uint32_t)target);
        }
    }

    spvc_context context = nullptr;
    spvc_compiler compiler = nullptr;
    spvc_compiler_options options = nullptr;
    spvc_parsed_ir ir = nullptr;

    spvc_context_create(&context);

    auto errorCallback = [](void* userdata, const char* error) { krypton::log::err("SPIRV-Cross error occured: {}", error); };
    auto checkCall = [](spvc_result result, const std::string& message) {
        if (result != spvc_result::SPVC_SUCCESS)
            krypton::log::err(message, result);
    };

    spvc_context_set_error_callback(context, errorCallback, nullptr);

    spvc_context_parse_spirv(context, spirv.data(), spirv.size(), &ir);
    spvc_context_create_compiler(context, backendTarget, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler);

    // Modify compiler options
    spvc_compiler_create_compiler_options(compiler, &options);
    switch (target) {
        case ShaderTargetType::GLSL:
            spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_GLSL_VERSION, 460);
            spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_FALSE);
            break;
        case ShaderTargetType::METAL:
            spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_MSL_VERSION, (2 << 16) + 4); /* MSL 2.4 */
            spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_MSL_ARGUMENT_BUFFERS, SPVC_TRUE);
            break;
        default: {
            break;
        }
    }
    spvc_compiler_install_compiler_options(compiler, options);

    ShaderCompileResult compileResult;
    const char* tempString;
    checkCall(spvc_compiler_compile(compiler, &tempString), "Failed to compile: {}");

    krypton::log::log("Address of compilation result: {}", fmt::ptr(&(*tempString)));
    if (tempString == nullptr) {
        krypton::log::throwError("Failed to compile through SPIRV-Cross: Compilation result was nullptr.");
    }

    compileResult.resultSize = std::strlen(tempString) * sizeof(uint8_t) + 1; // +1 for the null terminator char
    std::vector<uint8_t> newData(compileResult.resultSize);
    std::memcpy(newData.data(), tempString, compileResult.resultSize);
    compileResult.resultBytes = std::move(newData);

    compileResult.resultType = CompileResultType::String;

    spvc_context_release_allocations(context);
    spvc_context_destroy(context);

    return compileResult;
}

#ifdef WITH_SLANG_SHADERS
std::vector<krypton::shaders::ShaderCompileResult> krypton::shaders::slangCompileShader(const ShaderCompileInput& shaderInput) {
    SlangSession* session = spCreateSession(nullptr);

    SlangCompileRequest* request = spCreateCompileRequest(session);

    SlangCompileTarget compileTarget = SLANG_TARGET_UNKNOWN;
    switch (shaderInput.targetType) {
        using namespace krypton::shaders;
        case ShaderTargetType::GLSL:
            compileTarget = SLANG_GLSL;
            break;
        case ShaderTargetType::HLSL:
            compileTarget = SLANG_HLSL;
            break;
        case ShaderTargetType::SPIRV:
            compileTarget = SLANG_SPIRV;
            break;
        case ShaderTargetType::METAL: {
            krypton::log::throwError("[slang] Cannot use slang to compile to Metal");
            break;
        }
        default: {
            krypton::log::throwError("[slang] Unrecognized shader target type: {}", (uint32_t)shaderInput.targetType);
            break;
        }
    }
    auto target = spAddCodeGenTarget(request, compileTarget);

    SlangSourceLanguage compileSource = SLANG_SOURCE_LANGUAGE_UNKNOWN;
    switch (shaderInput.sourceType) {
        case ShaderSourceType::SLANG:
            compileSource = SLANG_SOURCE_LANGUAGE_SLANG;
            break;
        case ShaderSourceType::HLSL:
            compileSource = SLANG_SOURCE_LANGUAGE_HLSL;
            break;
        case ShaderSourceType::GLSL: {
            krypton::log::throwError("[slang] Cannot use slang to compile from GLSL!");
            break;
        }
        case ShaderSourceType::SPIRV: {
            krypton::log::throwError("[slang] Cannot use slang to compile from SPIR-V!");
            break;
        }
        default: {
            krypton::log::throwError("[slang] Unrecognized shader source type: {}", (uint32_t)shaderInput.sourceType);
            break;
        }
    }

    // This is our shaders directory relative to the executable.
    spAddSearchPath(request, "shaders/");

    // TODO: Increase debug info level for debug builds.
    spSetDebugInfoLevel(request, SLANG_DEBUG_INFO_LEVEL_NONE);

    spSetOptimizationLevel(request, SLANG_OPTIMIZATION_LEVEL_HIGH);

    // We, by default, use column major.
    spSetMatrixLayoutMode(request, SLANG_MATRIX_LAYOUT_COLUMN_MAJOR);

    spSetTargetForceGLSLScalarBufferLayout(request, target, true);

    // Could add macros using spAddPreprocessorDefine

    // Create translation unit; essentially a single file
    auto tuIndex = spAddTranslationUnit(request, compileSource, shaderInput.filePath.filename().string().c_str());
    spAddTranslationUnitSourceString(request, tuIndex, shaderInput.filePath.string().c_str(),
                                     reinterpret_cast<const char*>(shaderInput.source.data()));

    if (shaderInput.shaderStages.size() < shaderInput.entryPoints.size()) {
        krypton::log::throwError("[slang] Missing shader stages for entry points.");
    }

    std::vector<int> entryPointIndices;
    for (size_t i = 0; i < shaderInput.entryPoints.size(); ++i) {
        SlangStage slangStage;
        switch (shaderInput.shaderStages[i]) {
            case ShaderStage::Vertex:
                slangStage = SLANG_STAGE_VERTEX;
                break;
            case ShaderStage::Fragment:
                slangStage = SLANG_STAGE_FRAGMENT;
                break;
            case ShaderStage::Geometry:
                slangStage = SLANG_STAGE_GEOMETRY;
                break;
            case ShaderStage::Compute:
                slangStage = SLANG_STAGE_COMPUTE;
                break;
            case ShaderStage::Mesh:
                slangStage = SLANG_STAGE_MESH;
                break;
            case ShaderStage::RayGen:
                slangStage = SLANG_STAGE_RAY_GENERATION;
                break;
            case ShaderStage::ClosestHit:
                slangStage = SLANG_STAGE_CLOSEST_HIT;
                break;
            case ShaderStage::Miss:
                slangStage = SLANG_STAGE_MISS;
                break;
            case ShaderStage::AnyHit:
                slangStage = SLANG_STAGE_ANY_HIT;
                break;
            case ShaderStage::Intersect:
                slangStage = SLANG_STAGE_INTERSECTION;
                break;
            case ShaderStage::Callable:
                slangStage = SLANG_STAGE_CALLABLE;
                break;
            default: {
                krypton::log::throwError("[slang] Unrecognized shader stage: {}", static_cast<uint32_t>(shaderInput.shaderStages[i]));
                continue;
            }
        }

        entryPointIndices.push_back(spAddEntryPoint(request, tuIndex, shaderInput.entryPoints[i].c_str(), slangStage));
    }

    // Compile shaders
    int anyErrors = spCompile(request);
    const char* diagnostics = spGetDiagnosticOutput(request);

    if (anyErrors != 0) {
        std::istringstream ss(diagnostics);
        std::string line;
        while (std::getline(ss, line)) {
            krypton::log::err("{}: {}", shaderInput.filePath.string(), line);
        }
        return {};
    }

    std::vector<ShaderCompileResult> results;
    results.reserve(entryPointIndices.size());
    for (auto& entryPointIndex : entryPointIndices) {
        ShaderCompileResult compileResult = {};

        // Get the shader output code for this entry point.
        auto* data = spGetEntryPointCode(request, entryPointIndex, &compileResult.resultSize);
        if (shaderInput.targetType == ShaderTargetType::SPIRV) {
            VERIFY(compileResult.resultSize > 0 && compileResult.resultSize % 4 == 0);
        }

        // We have to reallocate the data as slang automatically deletes
        // the given string.
        std::vector<uint8_t> newData(compileResult.resultSize);
        std::memcpy(newData.data(), data, compileResult.resultSize);
        compileResult.resultBytes = std::move(newData);

        switch (shaderInput.targetType) {
            case ShaderTargetType::GLSL:
            case ShaderTargetType::HLSL: {
                compileResult.resultType = CompileResultType::String;
                break;
            }
            case ShaderTargetType::SPIRV: {
                compileResult.resultType = CompileResultType::Spirv;
                break;
            }
            default: {
                break;
            }
        }

        results.emplace_back(compileResult);
    }

    spDestroyCompileRequest(request);

    spDestroySession(session);

    return results;
}
#endif

std::vector<krypton::shaders::ShaderCompileResult> krypton::shaders::compileShaders(const fs::path& shaderFileName, ShaderStage shaderStage,
                                                                                    ShaderSourceType sourceType,
                                                                                    ShaderTargetType targetType) {
    if (!shaderFileName.has_filename()) {
        krypton::log::throwError("Given shader file path does not point to a file: {}", shaderFileName.string());
    }

    std::vector<uint8_t> source;
    if (sourceType == ShaderSourceType::SPIRV) {
        source = krypton::shaders::readBinaryShaderFile(shaderFileName);
    } else {
        auto shader = krypton::shaders::readShaderFile(shaderFileName);
        source = { shader.content.begin(), shader.content.end() };
    }

    krypton::shaders::ShaderCompileInput input = {
        .filePath = shaderFileName,
        .source = source,
        .entryPoints = { "main" },
        .shaderStages = { shaderStage },
        .sourceType = sourceType,
        .targetType = targetType,
    };

    return krypton::shaders::compileShaders(std::vector { input });
}

std::vector<krypton::shaders::ShaderCompileResult> krypton::shaders::compileShaders(const std::vector<ShaderCompileInput>& shaderInputs) {
    std::vector<krypton::shaders::ShaderCompileResult> results;
    for (auto& input : shaderInputs) {
        auto result = compileSingleShader(input);
        results.insert(results.end(), result.begin(), result.end());
    }
    return results;
}
