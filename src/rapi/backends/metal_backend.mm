#ifdef RAPI_WITH_METAL

#include <memory>
#include <vector>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <shader.hpp>

#include "metal_backend.hpp"

namespace krypton::rapi::metal {
    struct RenderObject final {
        std::shared_ptr<krypton::mesh::Mesh> mesh;
        id<MTLBuffer> vertexBuffer;
        id<MTLBuffer> indexBuffer;
        uint64_t vertexCount = 0, indexCount = 0;
        std::vector<uint64_t> bufferVertexOffsets;
        std::vector<uint64_t> bufferIndexOffsets;
    };
}

/**
 * Our rendering variables, they're made global as we
 * cannot put them in the non-Objective-C header
 */
id<MTLDevice> device;
id<MTLLibrary> library;
id<MTLCommandQueue> queue;
id<MTLRenderPipelineState> pipelineState;
CAMetalLayer* swapchain = nullptr;
NSWindow* nswindow = nullptr;
krypton::shaders::Shader defaultShader;

std::vector<krypton::rapi::metal::RenderObject> objects = {};

krypton::rapi::Metal_RAPI::Metal_RAPI() : window("Krypton", 1920, 1080) {

}

krypton::rapi::Metal_RAPI::~Metal_RAPI() {

}

void krypton::rapi::Metal_RAPI::drawFrame() {
    // Get the next image in the swapchain
    id<CAMetalDrawable> surface = [swapchain nextDrawable];

    // Create render pass
    MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0.25);
    pass.colorAttachments[0].loadAction  = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].texture = surface.texture;

    id<MTLCommandBuffer> buffer = [queue commandBuffer];
    id<MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:pass];

    // Submit each render object for rendering
    for (const krypton::rapi::metal::RenderObject& object : objects) {
        [encoder setRenderPipelineState:pipelineState];
        [encoder setVertexBuffer:object.vertexBuffer offset:0 atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:object.vertexCount];
        /*[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                 indexCount: object.indexCount
                  indexType: MTLIndexTypeUInt32
                indexBuffer: object.indexBuffer
          indexBufferOffset: 0];*/
        // Could also pass instanceCount and baseINstance to the above function
        // to instance rendering
    }

    // Present
    [encoder endEncoding];
    [buffer presentDrawable:surface];
    [buffer commit];
}

krypton::rapi::Window* krypton::rapi::Metal_RAPI::getWindow() {
    return &window;
}

void krypton::rapi::Metal_RAPI::init() {
    __autoreleasing NSError *error = nil;

    window.create(krypton::rapi::Backend::Metal);

    // Create the device, queue and metal layer
    device = MTLCreateSystemDefaultDevice();
    queue = [device newCommandQueue];
    swapchain = [CAMetalLayer layer];
    swapchain.device = device;
    swapchain.opaque = YES;
    swapchain.pixelFormat = MTLPixelFormatBGRA8Unorm;

    // Get the NSWindow*
    GLFWwindow* pWindow = window.getWindowPointer();
    nswindow = glfwGetCocoaWindow(pWindow);
    nswindow.contentView.layer = swapchain;
    nswindow.contentView.wantsLayer = YES;

    // Create the shaders
    defaultShader = krypton::shaders::readShaderFile("shaders/shaders.metal");
    NSString* shaderSource = [NSString stringWithUTF8String:defaultShader.content.c_str()];
    library = [device newLibraryWithSource:shaderSource options:0 error:&error];
    if(!library) {
        [NSException raise:@"Failed to compile shaders" format:@"%@", [error localizedDescription]];
    }

    // Create the pipeline
    id<MTLFunction> vertexProgram = [library newFunctionWithName:@"basic_vertex"];
    id<MTLFunction> fragmentProgram = [library newFunctionWithName:@"basic_fragment"];

    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    [pipelineStateDescriptor setVertexFunction:vertexProgram];
    [pipelineStateDescriptor setFragmentFunction:fragmentProgram];
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

    pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
}

void krypton::rapi::Metal_RAPI::render(std::shared_ptr<krypton::mesh::Mesh> mesh) {
    krypton::rapi::metal::RenderObject& renderObject
        = objects.emplace_back(krypton::rapi::metal::RenderObject {});
    uint64_t accumulatedVertexBufferSize = 0,
             accumulatedIndexBufferSize = 0;

    for (const auto& prim : mesh->primitives) {
        renderObject.vertexCount = prim.vertices.size();
        renderObject.indexCount = prim.indices.size();

        auto vertexDataSize = prim.vertices.size() * krypton::mesh::VERTEX_STRIDE;
        auto indexDataSize = prim.indices.size() * sizeof(krypton::mesh::Index);

        renderObject.bufferVertexOffsets.push_back(accumulatedVertexBufferSize + vertexDataSize);
         renderObject.bufferIndexOffsets.push_back(accumulatedIndexBufferSize  + indexDataSize);

        accumulatedVertexBufferSize += vertexDataSize;
        accumulatedIndexBufferSize += indexDataSize;
    }

    renderObject.vertexBuffer = [device newBufferWithLength:accumulatedVertexBufferSize options:0]; // 0 = shared buffer
    renderObject.indexBuffer = [device newBufferWithLength:accumulatedIndexBufferSize options:0];

    // Copy the vertex data for each primitive into the shared buffer.
    void* vertexData = [renderObject.vertexBuffer contents];
    void* indexData = [renderObject.indexBuffer contents];
    accumulatedVertexBufferSize = 0; accumulatedIndexBufferSize = 0;
    for (const auto& prim : mesh->primitives) {
        auto vertexDataSize = prim.vertices.size() * krypton::mesh::VERTEX_STRIDE; // vertex_stride == 32
        auto indexDataSize = prim.indices.size() * sizeof(krypton::mesh::Index); // index size == 32

        memcpy(reinterpret_cast<uint8_t*>(vertexData) + accumulatedVertexBufferSize, prim.vertices.data(), vertexDataSize);
        memcpy(reinterpret_cast<uint8_t*>(indexData) + accumulatedIndexBufferSize, prim.indices.data(), indexDataSize);

        accumulatedVertexBufferSize += vertexDataSize;
        accumulatedIndexBufferSize += indexDataSize;
    }
}

void krypton::rapi::Metal_RAPI::resize(int width, int height) {

}

void krypton::rapi::Metal_RAPI::shutdown() {

}

#endif // #ifdef RAPI_WITH_METAL
