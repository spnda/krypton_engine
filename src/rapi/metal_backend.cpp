#ifdef RAPI_WITH_METAL

#include <algorithm>

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

#include <rapi/metal/metal_layer_bridge.hpp>
#include <rapi/metal/shader_types.hpp>
#include <rapi/metal_backend.hpp>
#include <util/large_free_list.hpp>
#include <util/logging.hpp>

namespace krypton::rapi::metal {
    NS::String* createString(const std::string& string) {
        return NS::String::string(string.c_str(), NS::ASCIIStringEncoding);
    }
} // namespace krypton::rapi::metal

namespace kr = krypton::rapi;

kr::MetalBackend::MetalBackend() {
    window = std::make_shared<kr::Window>("Krypton", 1920, 1080);
}

kr::MetalBackend::~MetalBackend() = default;

void kr::MetalBackend::addPrimitive(krypton::util::Handle<"RenderObject">& handle, krypton::assets::Primitive& primitive,
                                    krypton::util::Handle<"Material">& material) {
    auto lock = std::scoped_lock(renderObjectMutex);
    auto& object = objects.getFromHandle(handle);
    object.primitives.push_back({ primitive, material, 0, 0, 0 });
}

void kr::MetalBackend::beginFrame() {
    ZoneScoped;
    window->pollEvents();

    window->newFrame();
    ImGui::NewFrame();

    /* Update camera data */
    void* cameraBufferData = cameraBuffer->contents();
    memcpy(cameraBufferData, cameraData.get(), kr::CAMERA_DATA_SIZE);
}

void kr::MetalBackend::buildMaterial(util::Handle<"Material">& handle) {}

void kr::MetalBackend::buildRenderObject(krypton::util::Handle<"RenderObject">& handle) {
    ZoneScoped;
    if (objects.isHandleValid(handle)) {
        kr::metal::RenderObject& renderObject = objects.getFromHandle(handle);

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

        renderObject.vertexBuffer = device->newBuffer(accumulatedVertexBufferSize, MTL::ResourceStorageModeManaged);
        renderObject.indexBuffer = device->newBuffer(accumulatedIndexBufferSize, MTL::ResourceStorageModeManaged);
        renderObject.instanceBuffer = device->newBuffer(sizeof(metal::InstanceData), MTL::ResourceStorageModeManaged);

        // Copy the vertex data for each primitive into the shared buffer.
        void* vertexData = renderObject.vertexBuffer->contents();
        void* indexData = renderObject.indexBuffer->contents();
        accumulatedVertexBufferSize = 0;
        accumulatedIndexBufferSize = 0;
        for (auto& primitive : renderObject.primitives) {
            const auto& prim = primitive.primitive;
            size_t vertexDataSize = prim.vertices.size() * krypton::assets::VERTEX_STRIDE;
            size_t indexDataSize = prim.indices.size() * sizeof(krypton::assets::Index); // index size == 32

            primitive.indexBufferOffset = accumulatedIndexBufferSize;
            primitive.indexCount = prim.indices.size();
            primitive.baseVertex = (int64_t)(accumulatedVertexBufferSize / krypton::assets::VERTEX_STRIDE);

            memcpy(reinterpret_cast<std::byte*>(vertexData) + accumulatedVertexBufferSize, prim.vertices.data(), vertexDataSize);
            memcpy(reinterpret_cast<std::byte*>(indexData) + accumulatedIndexBufferSize, prim.indices.data(), indexDataSize);

            accumulatedVertexBufferSize += vertexDataSize;
            accumulatedIndexBufferSize += indexDataSize;
        }

        metal::InstanceData instanceData;
        instanceData.instanceTransform = renderObject.transform; // conversion from 4x3 to 4x4
        memcpy(renderObject.instanceBuffer->contents(), &instanceData, renderObject.instanceBuffer->length());

        // As our vertex and index buffers are managed resources between the CPU and GPU, we'll
        // have to let Metal know to synchronize the contents of the buffers.
        // See https://developer.apple.com/documentation/metal/resource_fundamentals/synchronizing_a_managed_resource?language=objc
        renderObject.vertexBuffer->didModifyRange(NS::Range::Make(0, renderObject.vertexBuffer->length()));
        renderObject.indexBuffer->didModifyRange(NS::Range::Make(0, renderObject.indexBuffer->length()));
        renderObject.instanceBuffer->didModifyRange(NS::Range::Make(0, renderObject.instanceBuffer->length()));
    }
}

void kr::MetalBackend::createDepthTexture() {
    ZoneScoped;
    int width, height;
    window->getFramebufferSize(&width, &height);

    auto depthTexDesc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatDepth32Float_Stencil8, width, height, false);
    depthTexDesc->setStorageMode(MTL::StorageModePrivate);
    depthTexDesc->setUsage(MTL::TextureUsageRenderTarget);
    depthTexture = device->newTexture(depthTexDesc);
}

void kr::MetalBackend::createDeferredPipeline() {
    ZoneScoped;
    // Create depth stencil texture
    createDepthTexture();

    // Create textures for our G-Buffer
    createGBufferTextures();

    // Create our programs
    auto* vertexProgram = library->newFunction(metal::createString("basic_vertex"));
    auto* fragmentProgram = library->newFunction(metal::createString("basic_fragment"));
    vertexProgram->autorelease();
    fragmentProgram->autorelease();

    // Create the PSO
    auto* pipelineStateDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    pipelineStateDescriptor->setVertexFunction(vertexProgram);
    pipelineStateDescriptor->setFragmentFunction(fragmentProgram);
    pipelineStateDescriptor->colorAttachments()->object(0)->setPixelFormat(colorPixelFormat);
    pipelineStateDescriptor->colorAttachments()->object(1)->setPixelFormat(normalsTexture->pixelFormat());
    pipelineStateDescriptor->colorAttachments()->object(2)->setPixelFormat(albedoTexture->pixelFormat());
    pipelineStateDescriptor->setDepthAttachmentPixelFormat(depthTexture->pixelFormat());

    NS::Error* error = nullptr;
    pipelineState = device->newRenderPipelineState(pipelineStateDescriptor, &error);
    pipelineStateDescriptor->release();
    error->release();

    // Create our depth stencil
    auto* depthDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthDescriptor->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionLess);
    depthDescriptor->setDepthWriteEnabled(true);

    depthState = device->newDepthStencilState(depthDescriptor);
    depthDescriptor->release();
}

void kr::MetalBackend::createGBufferTextures() {
    ZoneScoped;
    int width, height;
    window->getFramebufferSize(&width, &height);

    auto* normalsDesc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA16Float, width, height, false);
    normalsDesc->setStorageMode(MTL::StorageModePrivate);
    normalsDesc->setUsage(MTL::TextureUsageRenderTarget);
    normalsTexture = device->newTexture(normalsDesc);

    auto* albedoDesc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm, width, height, false);
    albedoDesc->setStorageMode(MTL::StorageModePrivate);
    albedoDesc->setUsage(MTL::TextureUsageRenderTarget);
    albedoTexture = device->newTexture(albedoDesc);
}

krypton::util::Handle<"RenderObject"> kr::MetalBackend::createRenderObject() {
    ZoneScoped;
    auto mutex = std::scoped_lock<std::mutex>(renderObjectMutex);
    auto refCounter = std::make_shared<krypton::util::ReferenceCounter>();
    return objects.getNewHandle(refCounter);
}

krypton::util::Handle<"Material"> kr::MetalBackend::createMaterial(krypton::assets::Material material) {
    ZoneScoped;
    auto lock = std::scoped_lock(materialMutex);
    auto refCounter = std::make_shared<krypton::util::ReferenceCounter>();
    auto handle = materials.getNewHandle(refCounter);
    materials.getFromHandle(handle).baseColor = glm::fvec4(0.6);
    return handle;
}

bool kr::MetalBackend::destroyRenderObject(krypton::util::Handle<"RenderObject">& handle) {
    ZoneScoped;
    auto lock = std::scoped_lock(renderObjectMutex);
    auto valid = objects.isHandleValid(handle);
    if (valid) {
        auto& object = objects.getFromHandle(handle);

        objects.removeHandle(handle);
    } else {
        krypton::log::warn("Tried to destroy a invalid render object handle!");
    }
    return valid;
}

bool kr::MetalBackend::destroyMaterial(krypton::util::Handle<"Material">& handle) {
    auto lock = std::scoped_lock(materialMutex);
    auto valid = materials.isHandleValid(handle);
    if (valid) {
        materials.removeHandle(handle);
    } else {
        krypton::log::warn("Tried to destroy a invalid material handle!");
    }
    return valid;
}

void kr::MetalBackend::drawFrame() {
    ZoneScoped;

    auto* arPool = NS::AutoreleasePool::alloc()->init();
    auto* drawable = swapchain->nextDrawable();

    // Create render pass
    auto* mainPass = MTL::RenderPassDescriptor::renderPassDescriptor();

    auto* colorAttachment = mainPass->colorAttachments()->object(0);
    colorAttachment->setClearColor(MTL::ClearColor::Make(0, 0, 0, 0.25));
    colorAttachment->setLoadAction(MTL::LoadActionClear);
    colorAttachment->setStoreAction(MTL::StoreActionStore);
    colorAttachment->setTexture(drawable->texture());

    auto* normalsAttachment = mainPass->colorAttachments()->object(1);
    normalsAttachment->setClearColor(MTL::ClearColor::Make(0, 0, 0, 0));
    normalsAttachment->setLoadAction(MTL::LoadActionClear);
    normalsAttachment->setStoreAction(MTL::StoreActionStore);
    normalsAttachment->setTexture(normalsTexture);

    auto* albedoAttachment = mainPass->colorAttachments()->object(2);
    albedoAttachment->setClearColor(MTL::ClearColor::Make(0, 0, 0, 0));
    albedoAttachment->setLoadAction(MTL::LoadActionClear);
    albedoAttachment->setStoreAction(MTL::StoreActionStore);
    albedoAttachment->setTexture(albedoTexture);

    auto* depthAttachment = mainPass->depthAttachment();
    depthAttachment->setTexture(depthTexture);
    depthAttachment->setStoreAction(MTL::StoreActionDontCare);
    depthAttachment->setClearDepth(1.0f);

    // Create a command buffer and our initial command encoder
    auto* commandBuffer = queue->commandBuffer();
    auto* encoder = commandBuffer->renderCommandEncoder(mainPass);

    // Submit each render object for rendering
    encoder->setRenderPipelineState(pipelineState);
    encoder->setDepthStencilState(depthState);
    encoder->setVertexBuffer(cameraBuffer, 0, 2);

    for (const auto& handle : handlesForFrame) {
        const auto& object = objects.getFromHandle(handle);

        encoder->setVertexBuffer(object.vertexBuffer, 0, 0); /* Vertex buffer is at 0 */
        encoder->setVertexBuffer(object.instanceBuffer, 0, 1);

        for (const auto& primitive : object.primitives) {
            /* Dispatch draw calls */
            encoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, primitive.indexCount, MTL::IndexTypeUInt32, object.indexBuffer,
                                           primitive.indexBufferOffset, 1, primitive.baseVertex, 0);
        }
    }

    encoder->endEncoding();

    // Draw ImGui
    auto imguiColorAttachment = imGuiPassDescriptor->colorAttachments()->object(0);

    imguiColorAttachment->setLoadAction(MTL::LoadActionLoad);
    imguiColorAttachment->setStoreAction(MTL::StoreActionStore);
    imguiColorAttachment->setTexture(drawable->texture());

    ImGui_ImplMetal_NewFrame(imGuiPassDescriptor);
    ImGui::Render();
    auto* drawData = ImGui::GetDrawData();
    auto* imguiEncoder = commandBuffer->renderCommandEncoder(imGuiPassDescriptor);
    ImGui_ImplMetal_RenderDrawData(drawData, commandBuffer, imguiEncoder);
    imguiEncoder->endEncoding();

    // Present
    commandBuffer->presentDrawable(drawable);
    commandBuffer->commit();

    arPool->release();
}

void kr::MetalBackend::endFrame() {
    ZoneScoped;
    ImGui::EndFrame();

    handlesForFrame.clear();
}

std::shared_ptr<kr::CameraData> kr::MetalBackend::getCameraData() {
    return cameraData;
}

std::shared_ptr<kr::Window> kr::MetalBackend::getWindow() {
    return window;
}

void kr::MetalBackend::init() {
    ZoneScoped;
    window->create(kr::Backend::Metal);

    int w_width, w_height;
    window->getWindowSize(&w_width, &w_height);
    krypton::log::log("Window size: {}x{}", w_width, w_height);

    int width, height;
    window->getFramebufferSize(&width, &height);
    krypton::log::log("Framebuffer size: {}x{}", width, height);

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
    swapchain->setPixelFormat(colorPixelFormat);
    swapchain->setDrawableSize(drawableSize);

    krypton::log::log("Setting up Metal on {}", device->name()->utf8String());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    window->initImgui();
    ImGui_ImplMetal_Init(device);

    auto* pWindow = window->getWindowPointer();
    metal::setMetalLayerOnWindow(pWindow, swapchain);

    // Create our Metal library
    defaultShader = krypton::shaders::readShaderFile("shaders/shaders.metal");
    NS::String* shaderSource = NS::String::string(defaultShader.content.c_str(), NS::ASCIIStringEncoding);

    auto* compileOptions = MTL::CompileOptions::alloc()->init();
    compileOptions->setLanguageVersion(MTL::LanguageVersion2_4);

    NS::Error* error = nullptr;
    library = device->newLibrary(shaderSource, compileOptions, &error);
    if (!library) {
        krypton::log::err(error->description()->utf8String());
    }
    error->release();

    createDeferredPipeline();

    if (cameraData == nullptr)
        cameraData = std::make_shared<kr::CameraData>();

    cameraBuffer = device->newBuffer(kr::CAMERA_DATA_SIZE, MTL::ResourceStorageModeShared);

    imGuiPassDescriptor = MTL::RenderPassDescriptor::renderPassDescriptor();
}

void kr::MetalBackend::render(krypton::util::Handle<"RenderObject"> handle) {
    ZoneScoped;
    if (objects.isHandleValid(handle))
        handlesForFrame.push_back(handle);
}

void kr::MetalBackend::resize(int width, int height) {
    // Metal auto-resizes for us. Though we still have to re-recreate the deferred pipeline.
    depthTexture->release();
    normalsTexture->release();
    albedoTexture->release();

    createDeferredPipeline();
}

void kr::MetalBackend::setMaterialBaseColor(util::Handle<"Material">& handle, glm::fvec4 baseColor) {
    // todo
}

void kr::MetalBackend::setObjectName(krypton::util::Handle<"RenderObject">& handle, std::string name) {}

void kr::MetalBackend::setObjectTransform(krypton::util::Handle<"RenderObject">& handle, glm::mat4x3 transform) {
    ZoneScoped;
    auto lock = std::scoped_lock(renderObjectMutex);
    objects.getFromHandle(handle).transform = transform;
}

void kr::MetalBackend::shutdown() {
    ZoneScoped;
    albedoTexture->release();
    normalsTexture->release();
    depthTexture->release();

    imGuiPassDescriptor->release();
    ImGui_ImplMetal_DestroyDeviceObjects();
    queue->release();
    library->release();
    device->release();
}

#endif // #ifdef RAPI_WITH_METAL
