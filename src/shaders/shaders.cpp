#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <shaderc/shaderc.hpp>

#include "file_includer.hpp"
#include "shaders.hpp"

krypton::shaders::Shader krypton::shaders::readShaderFile(std::filesystem::path path) {
    std::ifstream is(path, std::ios::binary);

    if (is.is_open()) {
        std::string shaderCode;
        std::stringstream ss;
        ss << is.rdbuf();
        shaderCode = ss.str();
        return { path, ss.str() };
    } else {
        throw std::runtime_error(std::string("Failed to open shader file: ") + path.string());
    }
}

krypton::shaders::ShaderCompileResult krypton::shaders::compileGlslShader(
        const std::string& shaderName, const std::string& shaderSource, shaderc_shader_kind kind) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    // TODO: Macro definitions

    std::unique_ptr<FileIncluder> includer(new FileIncluder());
    options.SetIncluder(std::move(includer));
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    options.SetTargetSpirv(shaderc_spirv_version_1_5);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

    auto checkResult = [=]<typename T>(shaderc::CompilationResult<T>& result) mutable {
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << result.GetErrorMessage() << std::endl;
            return std::vector<T>();
        }
        return std::vector<T>(result.cbegin(),  result.cend());
    };

    // Preprocess the files.
    auto result = compiler.PreprocessGlsl(shaderSource, kind, shaderName.c_str(), options);
    const std::vector<char> preProcessedSourceChars = checkResult(result);
    const std::string preProcessedSource = {preProcessedSourceChars.begin(), preProcessedSourceChars.end()};

    // Compile to SPIR-V
#ifdef _DEBUG
    options.SetGenerateDebugInfo();
#endif // #ifdef _DEBUG
    auto compileResult = compiler.CompileGlslToSpv(preProcessedSource, kind, shaderName.c_str(), options);
    auto binary = checkResult(compileResult);

    options.SetGenerateDebugInfo();
    auto debugCompileResult = compiler.CompileGlslToSpv(preProcessedSource, kind, shaderName.c_str(), options);
    auto debugBinary = checkResult(debugCompileResult);
    return { binary, debugBinary };
}

krypton::shaders::CrossCompileResult krypton::shaders::crossCompile(const std::vector<uint32_t>& spirv, krypton::shaders::CrossCompileTarget target) {
    spvc_context context = nullptr;
    spvc_compiler compiler = nullptr;
    spvc_compiler_options options = nullptr;
    spvc_parsed_ir ir = nullptr;
    const char* result = nullptr;

    spvc_context_create(&context);
    spvc_context_parse_spirv(context, spirv.data(), spirv.size(), &ir);
    spvc_context_create_compiler(context, static_cast<spvc_backend>(target), ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler);

    // Modify compiler options
    spvc_compiler_create_compiler_options(compiler, &options);
    switch (target) {
        case CrossCompileTarget::TARGET_GLSL:
            spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_GLSL_VERSION, 330);
            spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_FALSE);
            break;
    }
    spvc_compiler_install_compiler_options(compiler, options);

    spvc_compiler_compile(compiler, &result);

    spvc_context_destroy(context);

    return { target, std::string { result }, };
}
