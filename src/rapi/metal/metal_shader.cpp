#include <Tracy.hpp>

#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_sampler.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <shaders/shader_types.hpp>
#include <util/bits.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

// clang-format off
constexpr std::array<std::pair<krypton::shaders::ShaderStage, MTL::RenderStages>, 2> metalShaderStages = {{
    {krypton::shaders::ShaderStage::Fragment, MTL::RenderStageFragment},
    {krypton::shaders::ShaderStage::Vertex, MTL::RenderStageVertex},
}};
// clang-format on

MTL::RenderStages kr::mtl::getRenderStages(shaders::ShaderStage stages) noexcept {
    MTL::RenderStages newStages = 0;
    for (const auto& [old_pos, new_pos] : metalShaderStages) {
        if (util::hasBit(stages, old_pos)) {
            newStages |= new_pos;
        }
    }
    return newStages;
}

#pragma region FragmentShader
kr::mtl::FragmentShader::FragmentShader(MTL::Device* device, std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType source)
    : IShader(bytes, source), device(device) {
    if (source != shaders::ShaderSourceType::METAL) {
        needs_transpile = true;
    }
}

kr::mtl::FragmentShader::FragmentShader(MTL::Device* device, std::vector<std::byte>&& bytes, krypton::shaders::ShaderSourceType source)
    : IShader(bytes, source), device(device) {
    if (source != shaders::ShaderSourceType::METAL) {
        needs_transpile = true;
    }
}

constexpr krypton::shaders::ShaderTargetType kr::mtl::FragmentShader::getTranspileTargetType() const {
    return shaders::ShaderTargetType::METAL;
}

void kr::mtl::FragmentShader::handleTranspileResult(krypton::shaders::ShaderCompileResult result) {
    msl = std::string { result.resultBytes.begin(), result.resultBytes.end() - 1 };
}

void kr::mtl::FragmentShader::createModule() {
    ZoneScoped;
    if (device == nullptr)
        return;

    auto* compileOptions = MTL::CompileOptions::alloc()->init();
    compileOptions->setLanguageVersion(MTL::LanguageVersion3_0);

    auto* content = getUTF8String(msl.c_str());

    NS::Error* error = nullptr;
    library = device->newLibrary(content, compileOptions, &error);
    if (library != nullptr) {
        kl::throwError(error->description()->utf8String());
    }
    compileOptions->release();

    function = library->newFunction(NSSTRING("main0"));
}

void kr::mtl::FragmentShader::destroy() {
    ZoneScoped;
    function->retain()->release();
}

MTL::Function* kr::mtl::FragmentShader::getFunction() const {
    return function;
}

bool kr::mtl::FragmentShader::isParameterObjectCompatible(IShaderParameter* parameter) {
    return true;
}

void kr::mtl::FragmentShader::setName(std::string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    if (function != nullptr)
        function->setLabel(name);
}
#pragma endregion

#pragma region VertexShader
kr::mtl::VertexShader::VertexShader(MTL::Device* device, std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType source)
    : IShader(bytes, source), device(device) {
    if (source != shaders::ShaderSourceType::METAL) {
        needs_transpile = true;
    }
}

kr::mtl::VertexShader::VertexShader(MTL::Device* device, std::vector<std::byte>&& bytes, krypton::shaders::ShaderSourceType source)
    : IShader(bytes, source), device(device) {
    if (source != shaders::ShaderSourceType::METAL) {
        needs_transpile = true;
    }
}

constexpr krypton::shaders::ShaderTargetType kr::mtl::VertexShader::getTranspileTargetType() const {
    return shaders::ShaderTargetType::METAL;
}

void kr::mtl::VertexShader::handleTranspileResult(krypton::shaders::ShaderCompileResult result) {
    msl = std::string { result.resultBytes.begin(), result.resultBytes.end() };
}

void kr::mtl::VertexShader::createModule() {
    ZoneScoped;
    if (device == nullptr)
        return;

    auto* compileOptions = MTL::CompileOptions::alloc()->init();
    compileOptions->setLanguageVersion(MTL::LanguageVersion3_0);

    auto* content = getUTF8String(msl.c_str());

    NS::Error* error = nullptr;
    library = device->newLibrary(content, compileOptions, &error);
    if (library == nullptr) {
        kl::throwError(error->description()->utf8String());
    }
    compileOptions->release();

    function = library->newFunction(NSSTRING("main0"));
}

void kr::mtl::VertexShader::destroy() {
    ZoneScoped;
    function->retain()->release();
}

MTL::Function* kr::mtl::VertexShader::getFunction() const {
    return function;
}

bool kr::mtl::VertexShader::isParameterObjectCompatible(IShaderParameter* parameter) {
    return true;
}

void kr::mtl::VertexShader::setName(std::string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    if (function != nullptr)
        function->setLabel(name);
}
#pragma endregion
