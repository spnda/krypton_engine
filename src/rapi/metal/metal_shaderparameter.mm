// We can't use ObjC's __bridge with the new C++ casts.
#pragma clang diagnostic ignored "-Wold-style-cast"

#import <Foundation/NSArray.h>
#import <Foundation/NSArray.hpp>
#import <Metal/MTLArgumentEncoder.hpp>
#import <Metal/MTLDevice.h> // For MTLArgumentDescriptor
#import <Tracy.hpp>

#import <rapi/metal/metal_buffer.hpp>
#import <rapi/metal/metal_cpp_util.hpp>
#import <rapi/metal/metal_sampler.hpp>
#import <rapi/metal/metal_shaderparameter.hpp>
#import <rapi/metal/metal_texture.hpp>
#import <util/logging.hpp>

namespace kr = krypton::rapi;

// clang-format off
constexpr std::array<MTLDataType, 7> metalDataTypes = {
    MTLDataTypeNone,
    MTLDataTypeSampler,
    MTLDataTypeTexture,
    MTLDataTypeTexture,
    MTLDataTypePointer,
    MTLDataTypePointer,
    MTLDataTypeInstanceAccelerationStructure,
};
// clang-format on

#pragma region mtl::ShaderParameterPool
kr::mtl::ShaderParameterPool::ShaderParameterPool(MTL::Device* device) noexcept : device(device) {}

bool kr::mtl::ShaderParameterPool::allocate(ShaderParameterLayout layout, std::unique_ptr<IShaderParameter>& parameter) {
    ZoneScoped;
    auto* descriptors = [[NSMutableArray alloc] initWithCapacity:layout.layoutInfo.bindings.size()];

    for (const auto& binding : layout.layoutInfo.bindings) {
        auto* descriptor = [[MTLArgumentDescriptor alloc] init];
        descriptor.index = binding.bindingId;
        descriptor.dataType = metalDataTypes[static_cast<uint16_t>(binding.type)];
        descriptor.access = MTLArgumentAccessReadWrite;
        if (descriptor.dataType == MTLDataTypeTexture) {
            descriptor.textureType = MTLTextureType2D;
        }
        [descriptors addObject:descriptor];
    }

    auto* encoder = device->newArgumentEncoder((__bridge NS::Array*)descriptors);
    auto* argumentBuffer = heap->newBuffer(encoder->encodedLength(), MTL::ResourceStorageModeShared);
    encoder->setArgumentBuffer(argumentBuffer, 0, 0);
    auto vParameter = std::make_unique<ShaderParameter>(device, encoder, argumentBuffer);
    parameter = std::move(vParameter);

    argumentBuffers.emplace_back(argumentBuffer);

    [descriptors removeAllObjects];
    [descriptors release];

    return true;
}

void kr::mtl::ShaderParameterPool::create() {
    ZoneScoped;
    auto* heapDescriptor = MTL::HeapDescriptor::alloc()->init();
    heapDescriptor->setType(MTL::HeapTypeAutomatic);
    heapDescriptor->setStorageMode(MTL::StorageModeShared);
    heapDescriptor->setSize(megabyte); // This should be enough for all argument buffers we allocate.
    heap = device->newHeap(heapDescriptor);
    heapDescriptor->release();

    if (name != nullptr) {
        heap->setLabel(name);
    }
}

void kr::mtl::ShaderParameterPool::destroy() {
    ZoneScoped;
    reset();
    heap->retain()->release();
}

void kr::mtl::ShaderParameterPool::reset() {
    ZoneScoped;
    for (auto& buffer : argumentBuffers) {
        buffer->retain()->release();
    }
}

void kr::mtl::ShaderParameterPool::setName(std::string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    if (heap != nullptr && name->length() != 0) {
        heap->setLabel(name);
    }
}
#pragma endregion

#pragma region mtl::ShaderParameter
kr::mtl::ShaderParameter::ShaderParameter(MTL::Device* device, MTL::ArgumentEncoder* encoder, MTL::Buffer* buffer)
    : device(device), encoder(encoder), argumentBuffer(buffer) {}

void kr::mtl::ShaderParameter::setBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) {
    ZoneScoped;
    buffers[index] = std::dynamic_pointer_cast<Buffer>(buffer);
}

void kr::mtl::ShaderParameter::setName(std::string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    if (name->length() != 0) {
        argumentBuffer->setLabel(name);
    }
}

void kr::mtl::ShaderParameter::setSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) {
    ZoneScoped;
    samplers[index] = std::dynamic_pointer_cast<Sampler>(sampler);
}

void kr::mtl::ShaderParameter::setTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) {
    ZoneScoped;
    textures[index] = std::dynamic_pointer_cast<Texture>(texture);
}

void kr::mtl::ShaderParameter::update() {
    ZoneScoped;
    for (auto& buf : buffers) {
        encoder->setBuffer(buf.second->buffer, 0, buf.first);
    }

    for (auto& tex : textures) {
        encoder->setTexture(tex.second->texture, tex.first);
    }

    for (auto& sam : samplers) {
        encoder->setSamplerState(sam.second->samplerState, sam.first);
    }

    buffers.clear();
    textures.clear();
    samplers.clear();
}
#pragma endregion
