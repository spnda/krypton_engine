#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <fmt/core.h>

#include <shaders/shaders.hpp>

krypton::shaders::Shader krypton::shaders::readShaderFile(std::filesystem::path path) {
    std::ifstream is(path, std::ios::binary);

    if (is.is_open()) {
        std::string shaderCode;
        std::stringstream ss;
        ss << is.rdbuf();
        shaderCode = ss.str();
        return {path, ss.str()};
    } else {
        throw std::runtime_error(std::string("Failed to open shader file: ") + path.string());
    }
}

krypton::shaders::ShaderCompileResult krypton::shaders::compileGlslShader(
    const std::string& shaderName, const std::string& shaderSource, ShaderStage shaderStage, TargetSpirv target) {
    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = static_cast<glslang_stage_t>(shaderStage),
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_2,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = static_cast<glslang_target_language_version_t>(target),
        .code = shaderSource.c_str(),
        .default_version = 460,
        .default_profile = GLSLANG_NO_PROFILE,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
    };

    glslang_initialize_process();

    glslang_shader_t* shader = glslang_shader_create(&input);
    if (!glslang_shader_preprocess(shader, &input)) {
    }
    if (!glslang_shader_parse(shader, &input)) {
    }
    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
    }

    glslang_program_SPIRV_generate(program, input.stage);
    if (glslang_program_SPIRV_get_messages(program)) {
        fmt::print(stderr, "{}", glslang_program_SPIRV_get_messages(program));
    }
    glslang_shader_delete(shader);

    auto* spvptr = glslang_program_SPIRV_get_ptr(program);
    std::vector<uint32_t> spv;
    spv.assign(spvptr,
               spvptr + glslang_program_SPIRV_get_size(program) * sizeof(uint32_t));
    return {spv, spv};
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
        case CrossCompileTarget::TARGET_METAL:
            spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_MSL_VERSION,
                                           (2 << 16) + 4); /* MSL 2.4 */
            break;
    }
    spvc_compiler_install_compiler_options(compiler, options);

    spvc_compiler_compile(compiler, &result);

    spvc_context_destroy(context);

    return {
        target,
        std::string {result},
    };
}
