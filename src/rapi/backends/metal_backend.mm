#ifdef RAPI_WITH_METAL

#include <memory>
#include <vector>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <imgui.h>
#include <imgui_impl_metal.h>

#include <rapi/backends/metal_backend.hpp>
#include <shaders/shaders.hpp>
#include <util/large_free_list.hpp>
#include <util/logging.hpp>

namespace krypton::rapi::metal {
    struct Primitive final {
        krypton::mesh::Primitive primitive = {};
        krypton::util::Handle<"Material"> material;
    };

    struct RenderObject final {
        std::vector<Primitive> primitives = {};

        id<MTLBuffer> vertexBuffer;
        id<MTLBuffer> indexBuffer;
        uint64_t totalVertexCount = 0, totalIndexCount = 0;
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
krypton::shaders::ShaderFile defaultShader;

id<MTLBuffer> cameraBuffer;

krypton::util::LargeFreeList<krypton::rapi::metal::RenderObject, "RenderObject"> objects = {};
std::mutex renderObjectMutex;

// Placeholder materials free list.
krypton::util::FreeList<uint32_t, "Material"> materials = {};
std::mutex materialMutex;

krypton::rapi::MetalBackend::MetalBackend() {
    window = std::make_shared<krypton::rapi::Window>("Krypton", 1920, 1080);
}

krypton::rapi::MetalBackend::~MetalBackend() = default;

void krypton::rapi::MetalBackend::addPrimitive(krypton::util::Handle<"RenderObject">& handle, krypton::mesh::Primitive& primitive,
                                               krypton::util::Handle<"Material">& material) {
    auto lock = std::scoped_lock(renderObjectMutex);
    auto& object = objects.getFromHandle(handle);
    object.primitives.push_back({primitive, material});
}

void krypton::rapi::MetalBackend::beginFrame() {
    window->pollEvents();
    window->newFrame();
    ImGui::NewFrame();

    /* Update camera data */
    void* cameraBufferData = [cameraBuffer contents];
    memcpy(cameraBufferData, cameraData.get(), krypton::rapi::CAMERA_DATA_SIZE);
}

void krypton::rapi::MetalBackend::buildRenderObject(krypton::util::Handle<"RenderObject">& handle) {
    if (objects.isHandleValid(handle)) {
        krypton::rapi::metal::RenderObject& renderObject = objects.getFromHandle(handle);

        size_t accumulatedVertexBufferSize = 0, accumulatedIndexBufferSize = 0;

        for (const auto& primitive : renderObject.primitives) {
            const auto& prim = primitive.primitive;
            renderObject.totalVertexCount += prim.vertices.size();
            renderObject.totalIndexCount += prim.indices.size();

            size_t vertexDataSize = prim.vertices.size() * krypton::mesh::VERTEX_STRIDE;
            size_t indexDataSize = prim.indices.size() * sizeof(krypton::mesh::Index);

            renderObject.bufferVertexOffsets.push_back(accumulatedVertexBufferSize + vertexDataSize);
            renderObject.bufferIndexOffsets.push_back(accumulatedIndexBufferSize + indexDataSize);

            accumulatedVertexBufferSize += vertexDataSize;
            accumulatedIndexBufferSize += indexDataSize;
        }

        renderObject.vertexBuffer = [device newBufferWithLength:accumulatedVertexBufferSize options:0]; // 0 = shared buffer
        renderObject.indexBuffer = [device newBufferWithLength:accumulatedIndexBufferSize options:0];

        // Copy the vertex data for each primitive into the shared buffer.
        void* vertexData = [renderObject.vertexBuffer contents];
        void* indexData = [renderObject.indexBuffer contents];
        accumulatedVertexBufferSize = 0;
        accumulatedIndexBufferSize = 0;
        for (const auto& primitive : renderObject.primitives) {
            const auto& prim = primitive.primitive;
            size_t vertexDataSize = prim.vertices.size() * krypton::mesh::VERTEX_STRIDE;
            size_t indexDataSize = prim.indices.size() * sizeof(krypton::mesh::Index); // index size == 32

            memcpy(reinterpret_cast<uint8_t*>(vertexData) + accumulatedVertexBufferSize, prim.vertices.data(), vertexDataSize);
            memcpy(reinterpret_cast<uint8_t*>(indexData) + accumulatedIndexBufferSize, prim.indices.data(), indexDataSize);

            accumulatedVertexBufferSize += vertexDataSize;
            accumulatedIndexBufferSize += indexDataSize;
        }
    }
}

krypton::util::Handle<"RenderObject"> krypton::rapi::MetalBackend::createRenderObject() {
    auto mutex = std::scoped_lock<std::mutex>(renderObjectMutex);
    auto refCounter = std::make_shared<krypton::util::ReferenceCounter>();
    return objects.getNewHandle(refCounter);
}

krypton::util::Handle<"Material"> krypton::rapi::MetalBackend::createMaterial(krypton::mesh::Material material) {
    auto lock = std::scoped_lock(materialMutex);
    auto refCounter = std::make_shared<krypton::util::ReferenceCounter>();
    return materials.getNewHandle(refCounter);
}

bool krypton::rapi::MetalBackend::destroyRenderObject(krypton::util::Handle<"RenderObject">& handle) {
    auto lock = std::scoped_lock(renderObjectMutex);
    auto valid = objects.isHandleValid(handle);
    if (valid) {
        auto& object = objects.getFromHandle(handle);

        objects.removeHandle(handle);
    }
    return valid;
}

bool krypton::rapi::MetalBackend::destroyMaterial(krypton::util::Handle<"Material">& handle) {}

void krypton::rapi::MetalBackend::drawFrame() {
    @autoreleasepool {
        id<CAMetalDrawable> surface = [swapchain nextDrawable];

        // Create render pass
        MTLRenderPassDescriptor* mainPass = [MTLRenderPassDescriptor renderPassDescriptor];
        mainPass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0.25);
        mainPass.colorAttachments[0].loadAction = MTLLoadActionClear;
        mainPass.colorAttachments[0].storeAction = MTLStoreActionStore;
        mainPass.colorAttachments[0].texture = surface.texture;

        id<MTLCommandBuffer> buffer = [queue commandBuffer];
        id<MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:mainPass];

        // Submit each render object for rendering
        [encoder setRenderPipelineState:pipelineState];
        [encoder setVertexBuffer:cameraBuffer offset:0 atIndex:1];

        for (krypton::util::Handle<"RenderObject">& handle : handlesForFrame) {
            krypton::rapi::metal::RenderObject& object = objects.getFromHandle(handle);

            [encoder setVertexBuffer:object.vertexBuffer offset:0 atIndex:0]; /* Vertex buffer is at 0 */

            /* Dispatch draw call */
            [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                indexCount:object.totalIndexCount
                                 indexType:MTLIndexTypeUInt32
                               indexBuffer:object.indexBuffer
                         indexBufferOffset:0];
            // Could also pass instanceCount and baseInstance to the above function
            // to instance rendering
        }

        [encoder endEncoding];

        // Draw ImGui
        MTLRenderPassDescriptor* imguiPass = [MTLRenderPassDescriptor renderPassDescriptor];
        imguiPass.colorAttachments[0].loadAction = MTLLoadActionLoad;
        imguiPass.colorAttachments[0].storeAction = MTLStoreActionStore;
        imguiPass.colorAttachments[0].texture = surface.texture;

        // The glfw imgui implementation which window->newFrame() calls
        // also updates the ImGuiIO for us.
        ImGui_ImplMetal_NewFrame(imguiPass);

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        id<MTLRenderCommandEncoder> imguiEncoder = [buffer renderCommandEncoderWithDescriptor:imguiPass];
        [imguiEncoder pushDebugGroup:@"Dear ImGui rendering"];
        ImGui_ImplMetal_RenderDrawData(drawData, buffer, imguiEncoder);
        [imguiEncoder popDebugGroup];

        // Present
        [imguiEncoder endEncoding];
        [buffer presentDrawable:surface];
        [buffer commit];
    }
}

void krypton::rapi::MetalBackend::endFrame() {
    ImGui::EndFrame();

    handlesForFrame.clear();
}

std::shared_ptr<krypton::rapi::CameraData> krypton::rapi::MetalBackend::getCameraData() {
    return cameraData;
}

std::shared_ptr<krypton::rapi::Window> krypton::rapi::MetalBackend::getWindow() {
    return window;
}

void krypton::rapi::MetalBackend::init() {
    __autoreleasing NSError* error = nil;

    window->create(krypton::rapi::Backend::Metal);

    int width, height;
    window->getFramebufferSize(&width, &height);

    // We have to set this because the window otherwise defaults to our
    // scaled size and not the actual wanted framebuffer size.
    CGSize drawableSize = {};
    drawableSize.width = (double)width;
    drawableSize.height = (double)height;

    // Create the device, queue and metal layer
    device = MTLCreateSystemDefaultDevice();
    queue = [device newCommandQueue];
    swapchain = [CAMetalLayer layer];
    swapchain.device = device;
    swapchain.opaque = YES;
    swapchain.pixelFormat = MTLPixelFormatBGRA8Unorm;
    swapchain.drawableSize = drawableSize;

    krypton::log::log("Setting up Metal on {}", [device.name UTF8String]);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();

    window->initImgui();

    ImGui_ImplMetal_Init(device);

    // Get the NSWindow*
    GLFWwindow* pWindow = window->getWindowPointer();
    nswindow = glfwGetCocoaWindow(pWindow);
    nswindow.contentView.layer = swapchain;
    nswindow.contentView.wantsLayer = YES;

    // Create the shaders
    defaultShader = krypton::shaders::readShaderFile("shaders/shaders.metal");
    NSString* shaderSource = [NSString stringWithUTF8String:defaultShader.content.c_str()];
    library = [device newLibraryWithSource:shaderSource options:0 error:&error];
    if (!library) {
        [NSException raise:@"Failed to compile shaders" format:@"%@", [error localizedDescription]];
    }

    // Create the pipeline
    id<MTLFunction> vertexProgram = [library newFunctionWithName:@"basic_vertex"];
    id<MTLFunction> fragmentProgram = [library newFunctionWithName:@"basic_fragment"];

    MTLRenderPipelineDescriptor* pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    [pipelineStateDescriptor setVertexFunction:vertexProgram];
    [pipelineStateDescriptor setFragmentFunction:fragmentProgram];
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

    pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];

    if (cameraData == nullptr)
        cameraData = std::make_shared<krypton::rapi::CameraData>();

    cameraBuffer = [device newBufferWithLength:krypton::rapi::CAMERA_DATA_SIZE options:0];
}

void krypton::rapi::MetalBackend::render(krypton::util::Handle<"RenderObject"> handle) {
    if (objects.isHandleValid(handle))
        handlesForFrame.push_back(handle);
}

void krypton::rapi::MetalBackend::resize(int width, int height) {
    // Metal auto-resizes for us.
}

void krypton::rapi::MetalBackend::setObjectName(krypton::util::Handle<"RenderObject">& handle, std::string name) {}

void krypton::rapi::MetalBackend::setObjectTransform(krypton::util::Handle<"RenderObject">& handle, glm::mat4x3 transform) {}

void krypton::rapi::MetalBackend::shutdown() {
    ImGui_ImplMetal_DestroyDeviceObjects();
}

#endif // #ifdef RAPI_WITH_METAL
