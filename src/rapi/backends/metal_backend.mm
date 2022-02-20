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
#include <rapi/render_object_handle.hpp>
#include <shaders/shaders.hpp>
#include <util/large_free_list.hpp>

namespace krypton::rapi::metal {
    struct RenderObject final {
        std::shared_ptr<krypton::mesh::Mesh> mesh;
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

krypton::util::LargeFreeList<krypton::rapi::metal::RenderObject> objects = {};

krypton::rapi::Metal_RAPI::Metal_RAPI() { window = std::make_shared<krypton::rapi::Window>("Krypton", 1920, 1080); }

krypton::rapi::Metal_RAPI::~Metal_RAPI() {}

void krypton::rapi::Metal_RAPI::beginFrame() {
    window->newFrame();
    ImGui::NewFrame();

    /** Update camera data */
    void* cameraBufferData = [cameraBuffer contents];
    memcpy(cameraBufferData, cameraData.get(), krypton::rapi::CAMERA_DATA_SIZE);
}

krypton::rapi::RenderObjectHandle krypton::rapi::Metal_RAPI::createRenderObject() {
    return static_cast<krypton::rapi::RenderObjectHandle>(objects.getNewHandle());
}

krypton::rapi::MaterialHandle krypton::rapi::Metal_RAPI::createMaterial(krypton::mesh::Material material) {}

bool krypton::rapi::Metal_RAPI::destroyRenderObject(krypton::rapi::RenderObjectHandle& handle) {
    auto valid = objects.isHandleValid(handle);
    if (valid) {
        auto& object = objects.getFromHandle(handle);

        objects.removeHandle(handle);
    }
    return valid;
}

void krypton::rapi::Metal_RAPI::drawFrame() {
    int width, height;
    window->getWindowSize(&width, &height);
    ImGui::GetIO().DisplaySize.x = width;
    ImGui::GetIO().DisplaySize.y = height;

    CGFloat framebufferScale = nswindow.screen.backingScaleFactor ?: NSScreen.mainScreen.backingScaleFactor;
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(framebufferScale, framebufferScale);

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
        for (krypton::rapi::RenderObjectHandle& handle : handlesForFrame) {
            krypton::rapi::metal::RenderObject& object = objects.getFromHandle(handle);

            [encoder setRenderPipelineState:pipelineState];
            [encoder setVertexBuffer:object.vertexBuffer offset:0 atIndex:0]; /* Vertex buffer is at 0 */
            [encoder setVertexBuffer:cameraBuffer offset:0 atIndex:1];        /* Camera buffer is at 1 */

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
        ImGui_ImplMetal_NewFrame(imguiPass); // It's safe to call now

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

void krypton::rapi::Metal_RAPI::endFrame() {
    ImGui::EndFrame();

    handlesForFrame.clear();
}

std::shared_ptr<krypton::rapi::CameraData> krypton::rapi::Metal_RAPI::getCameraData() { return cameraData; }

std::shared_ptr<krypton::rapi::Window> krypton::rapi::Metal_RAPI::getWindow() { return window; }

void krypton::rapi::Metal_RAPI::init() {
    __autoreleasing NSError* error = nil;

    window->create(krypton::rapi::Backend::Metal);

    // Create the device, queue and metal layer
    device = MTLCreateSystemDefaultDevice();
    queue = [device newCommandQueue];
    swapchain = [CAMetalLayer layer];
    swapchain.device = device;
    swapchain.opaque = YES;
    swapchain.pixelFormat = MTLPixelFormatBGRA8Unorm;

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

void krypton::rapi::Metal_RAPI::loadMeshForRenderObject(krypton::rapi::RenderObjectHandle& handle,
                                                        std::shared_ptr<krypton::mesh::Mesh> mesh) {
    if (objects.isHandleValid(handle)) {
        krypton::rapi::metal::RenderObject& renderObject = objects.getFromHandle(handle);

        size_t accumulatedVertexBufferSize = 0, accumulatedIndexBufferSize = 0;

        for (const auto& prim : mesh->primitives) {
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
        for (const auto& prim : mesh->primitives) {
            size_t vertexDataSize = prim.vertices.size() * krypton::mesh::VERTEX_STRIDE;
            size_t indexDataSize = prim.indices.size() * sizeof(krypton::mesh::Index); // index size == 32

            memcpy(reinterpret_cast<uint8_t*>(vertexData) + accumulatedVertexBufferSize, prim.vertices.data(), vertexDataSize);
            memcpy(reinterpret_cast<uint8_t*>(indexData) + accumulatedIndexBufferSize, prim.indices.data(), indexDataSize);

            accumulatedVertexBufferSize += vertexDataSize;
            accumulatedIndexBufferSize += indexDataSize;
        }
    }
}

void krypton::rapi::Metal_RAPI::render(RenderObjectHandle handle) {
    if (objects.isHandleValid(handle))
        handlesForFrame.push_back(handle);
}

void krypton::rapi::Metal_RAPI::resize(int width, int height) {
    // Metal auto-resizes for us.
}

void krypton::rapi::Metal_RAPI::shutdown() { ImGui_ImplMetal_DestroyDeviceObjects(); }

#endif // #ifdef RAPI_WITH_METAL
