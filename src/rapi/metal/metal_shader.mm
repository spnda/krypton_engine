#import <Foundation/Foundation.h>
#import <Foundation/NSArray.h>
#import <Metal/MTLArgumentEncoder.hpp>
#import <Metal/Metal.h>
#import <Tracy.hpp>

#import <rapi/metal/metal_buffer.hpp>
#import <rapi/metal/metal_cpp_util.hpp>
#import <rapi/metal/metal_sampler.hpp>
#import <rapi/metal/metal_shader.hpp>
#import <rapi/metal/metal_texture.hpp>
#import <shaders/shader_types.hpp>
#import <util/logging.hpp>

namespace kr = krypton::rapi;

#pragma region ShaderParameter
kr::mtl::ShaderParameter::ShaderParameter(MTL::Device* device) : device(device) {}

void kr::mtl::ShaderParameter::buildParameter() {
    ZoneScoped;
    // I've placed this function definition in this Obj-C++ file, as creating a NS::Array using
    // the metal-cpp wrappers was really not nice, and it is far simpler to just use Obj-C directly
    // in this case.
    auto* descriptors = [[NSMutableArray alloc] init];
    for (auto& buf : buffers) {
        auto* descriptor = [MTLArgumentDescriptor argumentDescriptor];
        descriptor.index = buf.first;
        descriptor.dataType = MTLDataTypePointer;
        descriptor.access = MTLArgumentAccessReadWrite;
        [descriptors addObject:descriptor];
    }

    for (auto& tex : textures) {
        auto* descriptor = [MTLArgumentDescriptor argumentDescriptor];
        descriptor.index = tex.first;
        descriptor.dataType = MTLDataTypeTexture;
        descriptor.textureType = MTLTextureType2D;
        descriptor.access = MTLArgumentAccessReadWrite;
        [descriptors addObject:descriptor];
    }

    for (auto& sam : samplers) {
        auto* descriptor = [MTLArgumentDescriptor argumentDescriptor];
        descriptor.index = sam.first;
        descriptor.dataType = MTLDataTypeSampler;
        [descriptors addObject:descriptor];
    }

    encoder = device->newArgumentEncoder((__bridge NS::Array*)descriptors);

    argumentBuffer = device->newBuffer(encoder->encodedLength(), MTL::ResourceStorageModeShared);
    if (name != nullptr)
        argumentBuffer->setLabel(name);
    encoder->setArgumentBuffer(argumentBuffer, 0, 0);

    for (auto& buf : buffers) {
        encoder->setBuffer(buf.second->buffer, 0, buf.first);
    }

    for (auto& tex : textures) {
        encoder->setTexture(tex.second->texture, tex.first);
    }

    for (auto& sam : samplers) {
        encoder->setSamplerState(sam.second->samplerState, sam.first);
    }

    [descriptors release];
}

void kr::mtl::ShaderParameter::addBuffer(uint32_t index, std::shared_ptr<IBuffer> buffer) {
    buffers[index] = std::dynamic_pointer_cast<mtl::Buffer>(buffer);
}

void kr::mtl::ShaderParameter::addTexture(uint32_t index, std::shared_ptr<ITexture> texture) {
    textures[index] = std::dynamic_pointer_cast<mtl::Texture>(texture);
}

void kr::mtl::ShaderParameter::addSampler(uint32_t index, std::shared_ptr<ISampler> sampler) {
    samplers[index] = std::dynamic_pointer_cast<mtl::Sampler>(sampler);
}

void kr::mtl::ShaderParameter::destroy() {
    ZoneScoped;
    encoder->release();
    argumentBuffer->release();
}

void kr::mtl::ShaderParameter::setName(std::string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    if (argumentBuffer != nullptr)
        argumentBuffer->setLabel(name);
}
#pragma endregion

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
    msl = std::string{result.resultBytes.begin(), result.resultBytes.end() - 1};
}

void kr::mtl::FragmentShader::createModule() {
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
    msl = std::string{result.resultBytes.begin(), result.resultBytes.end()};
}

void kr::mtl::VertexShader::createModule() {
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
