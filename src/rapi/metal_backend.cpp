#ifdef RAPI_WITH_METAL

// These definitions need to be at the top so that the objective-c classes
// and selectors are properly created.
#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <Tracy.hpp>
#include <imgui.h>
#include <imgui_impl_metal.h>

#include <rapi/metal/CAMetalLayer.hpp>
#include <rapi/metal/metal_layer_bridge.hpp>
#include <rapi/metal_backend.hpp>
#include <util/large_free_list.hpp>
#include <util/logging.hpp>

namespace krypton::rapi::metal {
    struct Primitive final {
        krypton::assets::Primitive primitive = {};
        krypton::util::Handle<"Material"> material;
    };

    struct RenderObject final {
        std::vector<Primitive> primitives = {};

        MTL::Buffer* vertexBuffer;
        MTL::Buffer* indexBuffer;
        uint64_t totalVertexCount = 0, totalIndexCount = 0;
        std::vector<uint64_t> bufferVertexOffsets;
        std::vector<uint64_t> bufferIndexOffsets;
    };

    NS::String* createString(const std::string& string) {
        return NS::String::string(string.c_str(), NS::ASCIIStringEncoding);
    }
} // namespace krypton::rapi::metal

krypton::util::LargeFreeList<krypton::rapi::metal::RenderObject, "RenderObject"> objects = {};
std::mutex renderObjectMutex;

// Placeholder materials free list.
krypton::util::FreeList<uint32_t, "Material"> materials = {};
std::mutex materialMutex;

krypton::rapi::MetalBackend::MetalBackend() {
    window = std::make_shared<krypton::rapi::Window>("Krypton", 1920, 1080);
}

krypton::rapi::MetalBackend::~MetalBackend() = default;

void krypton::rapi::MetalBackend::addPrimitive(krypton::util::Handle<"RenderObject">& handle, krypton::assets::Primitive& primitive,
                                               krypton::util::Handle<"Material">& material) {
    auto lock = std::scoped_lock(renderObjectMutex);
    auto& object = objects.getFromHandle(handle);
    object.primitives.push_back({ primitive, material });
}

void krypton::rapi::MetalBackend::beginFrame() {
    ZoneScoped;
    window->pollEvents();

    window->newFrame();
    ImGui::NewFrame();

    /* Update camera data */
    void* cameraBufferData = cameraBuffer->contents();
    memcpy(cameraBufferData, cameraData.get(), krypton::rapi::CAMERA_DATA_SIZE);
}

void krypton::rapi::MetalBackend::buildRenderObject(krypton::util::Handle<"RenderObject">& handle) {
    ZoneScoped;
    if (objects.isHandleValid(handle)) {
        krypton::rapi::metal::RenderObject& renderObject = objects.getFromHandle(handle);

        size_t accumulatedVertexBufferSize = 0, accumulatedIndexBufferSize = 0;

        for (const auto& primitive : renderObject.primitives) {
            const auto& prim = primitive.primitive;
            renderObject.totalVertexCount += prim.vertices.size();
            renderObject.totalIndexCount += prim.indices.size();

            size_t vertexDataSize = prim.vertices.size() * krypton::assets::VERTEX_STRIDE;
            size_t indexDataSize = prim.indices.size() * sizeof(krypton::assets::Index);

            renderObject.bufferVertexOffsets.push_back(accumulatedVertexBufferSize + vertexDataSize);
            renderObject.bufferIndexOffsets.push_back(accumulatedIndexBufferSize + indexDataSize);

            accumulatedVertexBufferSize += vertexDataSize;
            accumulatedIndexBufferSize += indexDataSize;
        }

        renderObject.vertexBuffer = device->newBuffer(accumulatedVertexBufferSize, MTL::ResourceStorageModeShared);
        renderObject.indexBuffer = device->newBuffer(accumulatedIndexBufferSize, MTL::ResourceStorageModeShared);

        // Copy the vertex data for each primitive into the shared buffer.
        void* vertexData = renderObject.vertexBuffer->contents();
        void* indexData = renderObject.indexBuffer->contents();
        accumulatedVertexBufferSize = 0;
        accumulatedIndexBufferSize = 0;
        for (const auto& primitive : renderObject.primitives) {
            const auto& prim = primitive.primitive;
            size_t vertexDataSize = prim.vertices.size() * krypton::assets::VERTEX_STRIDE;
            size_t indexDataSize = prim.indices.size() * sizeof(krypton::assets::Index); // index size == 32

            memcpy(reinterpret_cast<uint8_t*>(vertexData) + accumulatedVertexBufferSize, prim.vertices.data(), vertexDataSize);
            memcpy(reinterpret_cast<uint8_t*>(indexData) + accumulatedIndexBufferSize, prim.indices.data(), indexDataSize);

            accumulatedVertexBufferSize += vertexDataSize;
            accumulatedIndexBufferSize += indexDataSize;
        }
    }
}

krypton::util::Handle<"RenderObject"> krypton::rapi::MetalBackend::createRenderObject() {
    ZoneScoped;
    auto mutex = std::scoped_lock<std::mutex>(renderObjectMutex);
    auto refCounter = std::make_shared<krypton::util::ReferenceCounter>();
    return objects.getNewHandle(refCounter);
}

krypton::util::Handle<"Material"> krypton::rapi::MetalBackend::createMaterial(krypton::assets::Material material) {
    ZoneScoped;
    auto lock = std::scoped_lock(materialMutex);
    auto refCounter = std::make_shared<krypton::util::ReferenceCounter>();
    return materials.getNewHandle(refCounter);
}

bool krypton::rapi::MetalBackend::destroyRenderObject(krypton::util::Handle<"RenderObject">& handle) {
    ZoneScoped;
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
    ZoneScoped;
    CA::MetalDrawable* surface = swapchain->nextDrawable();

    // Create render pass
    MTL::RenderPassDescriptor* mainPass = MTL::RenderPassDescriptor::renderPassDescriptor();
    auto colorAttachment = mainPass->colorAttachments()->object(0);

    colorAttachment->setClearColor(MTL::ClearColor::Make(0, 0, 0, 0.25));
    colorAttachment->setLoadAction(MTL::LoadActionClear);
    colorAttachment->setStoreAction(MTL::StoreActionStore);
    colorAttachment->setTexture(surface->texture());

    MTL::CommandBuffer* buffer = queue->commandBuffer();
    MTL::RenderCommandEncoder* encoder = buffer->renderCommandEncoder(mainPass);

    // Submit each render object for rendering
    encoder->setRenderPipelineState(pipelineState);
    encoder->setVertexBuffer(cameraBuffer, 0, 1);

    for (krypton::util::Handle<"RenderObject">& handle : handlesForFrame) {
        krypton::rapi::metal::RenderObject& object = objects.getFromHandle(handle);

        encoder->setVertexBuffer(object.vertexBuffer, 0, 0); /* Vertex buffer is at 0 */

        /* Dispatch draw call */
        encoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, object.totalIndexCount, MTL::IndexTypeUInt32, object.indexBuffer, 0);
        // Could also pass instanceCount and baseInstance to the above function
        // to instance rendering
    }

    encoder->endEncoding();

    // Draw ImGui
    auto imguiColorAttachment = imGuiPassDescriptor->colorAttachments()->object(0);

    imguiColorAttachment->setLoadAction(MTL::LoadActionLoad);
    imguiColorAttachment->setStoreAction(MTL::StoreActionStore);
    imguiColorAttachment->setTexture(surface->texture());

    ImGui_ImplMetal_NewFrame(imGuiPassDescriptor);
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    MTL::RenderCommandEncoder* imguiEncoder = buffer->renderCommandEncoder(imGuiPassDescriptor);
    ImGui_ImplMetal_RenderDrawData(drawData, buffer, imguiEncoder);
    imguiEncoder->endEncoding();

    // Present
    buffer->presentDrawable(surface);
    buffer->commit();

    imguiEncoder->release();
    encoder->release();
    buffer->release();
    surface->release();
    mainPass->release();
}

void krypton::rapi::MetalBackend::endFrame() {
    ZoneScoped;
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
    ZoneScoped;
    // __autoreleasing NSError* error = nil;
    NS::Error* error = nullptr;

    window->create(krypton::rapi::Backend::Metal);

    int width, height;
    window->getFramebufferSize(&width, &height);

    // We have to set this because the window otherwise defaults to our
    // scaled size and not the actual wanted framebuffer size.
    CG::Size drawableSize = {
        .width = (double)width,
        .height = (double)height,
    };

    // Create the device, queue and metal layer
    device = MTL::CreateSystemDefaultDevice();
    queue = device->newCommandQueue();
    swapchain = CA::MetalLayer::layer();
    swapchain->setDevice(device);
    swapchain->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    swapchain->setDrawableSize(drawableSize);

    krypton::log::log("Setting up Metal on {}", device->name()->utf8String());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    window->initImgui();
    ImGui_ImplMetal_Init(device);

    GLFWwindow* pWindow = window->getWindowPointer();
    metal::setMetalLayerOnWindow(pWindow, swapchain);

    // Create the shaders
    defaultShader = krypton::shaders::readShaderFile("shaders/shaders.metal");
    NS::String* shaderSource = metal::createString(defaultShader.content);
    library = device->newLibrary(shaderSource, nullptr, &error);
    if (!library) {
        // raise exception somehow else?
        // [NSException raise:@"Failed to compile shaders" format:@"%@", [error localizedDescription]];
    }

    // Create the pipeline
    MTL::Function* vertexProgram = library->newFunction(metal::createString("basic_vertex"));
    MTL::Function* fragmentProgram = library->newFunction(metal::createString("basic_fragment"));
    vertexProgram->autorelease();
    fragmentProgram->autorelease();

    MTL::RenderPipelineDescriptor* pipelineStateDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    pipelineStateDescriptor->setVertexFunction(vertexProgram);
    pipelineStateDescriptor->setFragmentFunction(fragmentProgram);
    pipelineStateDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    pipelineState = device->newRenderPipelineState(pipelineStateDescriptor, &error);
    pipelineStateDescriptor->release();

    if (cameraData == nullptr)
        cameraData = std::make_shared<krypton::rapi::CameraData>();

    cameraBuffer = device->newBuffer(krypton::rapi::CAMERA_DATA_SIZE, MTL::ResourceStorageModeShared);

    imGuiPassDescriptor = MTL::RenderPassDescriptor::renderPassDescriptor();

    error->release();
}

void krypton::rapi::MetalBackend::render(krypton::util::Handle<"RenderObject"> handle) {
    ZoneScoped;
    if (objects.isHandleValid(handle))
        handlesForFrame.push_back(handle);
}

void krypton::rapi::MetalBackend::resize(int width, int height) {
    // Metal auto-resizes for us.
}

void krypton::rapi::MetalBackend::setObjectName(krypton::util::Handle<"RenderObject">& handle, std::string name) {}

void krypton::rapi::MetalBackend::setObjectTransform(krypton::util::Handle<"RenderObject">& handle, glm::mat4x3 transform) {}

void krypton::rapi::MetalBackend::shutdown() {
    ZoneScoped;
    imGuiPassDescriptor->release();
    ImGui_ImplMetal_DestroyDeviceObjects();
    queue->release();
    library->release();
    device->release();
}

#endif // #ifdef RAPI_WITH_METAL
