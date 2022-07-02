#include <Metal/MTLDevice.hpp>
#include <Tracy.hpp>

#include <rapi/metal/metal_buffer.hpp>
#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_sampler.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

#pragma region ShaderParameter
// Note that the definition for ShaderParameter::buildParameter is within metal_shader.mm
kr::metal::ShaderParameter::ShaderParameter(MTL::Device* device) : device(device) {}

void kr::metal::ShaderParameter::addBuffer(uint32_t index, std::shared_ptr<IBuffer> buffer) {
    buffers[index] = std::dynamic_pointer_cast<metal::Buffer>(buffer);
}

void kr::metal::ShaderParameter::addTexture(uint32_t index, std::shared_ptr<ITexture> texture) {
    textures[index] = std::dynamic_pointer_cast<metal::Texture>(texture);
}

void kr::metal::ShaderParameter::addSampler(uint32_t index, std::shared_ptr<ISampler> sampler) {
    samplers[index] = std::dynamic_pointer_cast<metal::Sampler>(sampler);
}

void kr::metal::ShaderParameter::destroy() {
    encoder->release();
    argumentBuffer->release();
}
#pragma endregion

#pragma region FragmentShader
kr::metal::FragmentShader::FragmentShader(MTL::Device* device, std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType source)
    : IShader(bytes, source), device(device) {
    if (source != shaders::ShaderSourceType::METAL) {
        needs_transpile = true;
    }
}

kr::metal::FragmentShader::FragmentShader(MTL::Device* device, std::vector<std::byte>&& bytes, krypton::shaders::ShaderSourceType source)
    : IShader(bytes, source), device(device) {
    if (source != shaders::ShaderSourceType::METAL) {
        needs_transpile = true;
    }
}

constexpr krypton::shaders::ShaderTargetType kr::metal::FragmentShader::getTranspileTargetType() const {
    return shaders::ShaderTargetType::METAL;
}

void kr::metal::FragmentShader::handleTranspileResult(krypton::shaders::ShaderCompileResult result) {
    msl = std::string { result.resultBytes.begin(), result.resultBytes.end() - 1 };
}

void kr::metal::FragmentShader::createModule() {
    ZoneScoped;
    if (device == nullptr)
        return;

    auto* compileOptions = MTL::CompileOptions::alloc()->init();
    compileOptions->setLanguageVersion(MTL::LanguageVersion2_4);

    auto* content = NS::String::string(msl.c_str(), NS::ASCIIStringEncoding);

    NS::Error* error = nullptr;
    library = device->newLibrary(content, compileOptions, &error);
    if (!library) {
        krypton::log::err(error->description()->utf8String());
    }
    compileOptions->release();

    function = library->newFunction(NSSTRING("main0"));
}

bool kr::metal::FragmentShader::isParameterObjectCompatible(IShaderParameter* parameter) {
    return true;
}

void kr::metal::FragmentShader::setName(std::string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    if (function != nullptr)
        function->setLabel(name);
}
#pragma endregion

#pragma region VertexShader
kr::metal::VertexShader::VertexShader(MTL::Device* device, std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType source)
    : IShader(bytes, source), device(device) {
    if (source != shaders::ShaderSourceType::METAL) {
        needs_transpile = true;
    }
}

kr::metal::VertexShader::VertexShader(MTL::Device* device, std::vector<std::byte>&& bytes, krypton::shaders::ShaderSourceType source)
    : IShader(bytes, source), device(device) {
    if (source != shaders::ShaderSourceType::METAL) {
        needs_transpile = true;
    }
}

constexpr krypton::shaders::ShaderTargetType kr::metal::VertexShader::getTranspileTargetType() const {
    return shaders::ShaderTargetType::METAL;
}

void kr::metal::VertexShader::handleTranspileResult(krypton::shaders::ShaderCompileResult result) {
    msl = std::string { result.resultBytes.begin(), result.resultBytes.end() };
}

void kr::metal::VertexShader::createModule() {
    ZoneScoped;
    if (device == nullptr)
        return;

    auto* compileOptions = MTL::CompileOptions::alloc()->init();
    compileOptions->setLanguageVersion(MTL::LanguageVersion2_4);

    auto* content = NS::String::string(msl.c_str(), NS::ASCIIStringEncoding);

    NS::Error* error = nullptr;
    library = device->newLibrary(content, compileOptions, &error);
    if (!library) {
        krypton::log::err(error->description()->utf8String());
    }
    compileOptions->release();

    function = library->newFunction(NSSTRING("main0"));
}

bool kr::metal::VertexShader::isParameterObjectCompatible(IShaderParameter* parameter) {
    return true;
}

void kr::metal::VertexShader::setName(std::string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    if (function != nullptr)
        function->setLabel(name);
}
#pragma endregion
