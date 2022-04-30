#import <Foundation/Foundation.h>
#import <Metal/MTLArgumentEncoder.hpp>
#import <Metal/Metal.h>
#import <Tracy.hpp>

#import <rapi/metal/metal_cpp_util.hpp>
#import <rapi/metal/metal_shader.hpp>

void krypton::rapi::metal::ShaderParameter::buildParameter() {
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
    argumentBuffer->setLabel(NSSTRING("Argument Buffer"));
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
