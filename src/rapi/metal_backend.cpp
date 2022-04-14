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

#include <rapi/fsr_util.hpp>
#include <rapi/metal/metal_layer_bridge.hpp>
#include <rapi/metal/shader_types.hpp>
#include <rapi/metal_backend.hpp>
#include <util/large_free_list.hpp>
#include <util/logging.hpp>

namespace krypton::rapi::metal {
    NS::String* createString(const std::string& string) {
        return NS::String::string(string.c_str(), NS::ASCIIStringEncoding);
    }

    MTL::PixelFormat getPixelFormat(krypton::rapi::TextureFormat textureFormat, krypton::rapi::ColorEncoding colorEncoding) {
        switch (textureFormat) {
            case TextureFormat::RGBA8: {
                if (colorEncoding == ColorEncoding::LINEAR) {
                    return MTL::PixelFormatRGBA8Unorm;
                } else {
                    return MTL::PixelFormatRGBA8Unorm_sRGB;
                }
                break;
            }
            case TextureFormat::RGBA16: {
                if (colorEncoding == ColorEncoding::LINEAR) {
                    return MTL::PixelFormatRGBA16Unorm;
                }
                // There's no RGBA16Unorm_sRGB
                break;
            }
            case TextureFormat::A8: {
                if (colorEncoding == ColorEncoding::LINEAR) {
                    return MTL::PixelFormatA8Unorm;
                }
                // There's no A8Unorm_sRGB
                break;
            }
        }

        return MTL::PixelFormatInvalid;
    }
} // namespace krypton::rapi::metal

namespace kr = krypton::rapi;
namespace ku = krypton::util;

kr::MetalBackend::MetalBackend() {
    window = std::make_shared<kr::Window>("Krypton", 1920, 1080);
}

kr::MetalBackend::~MetalBackend() = default;

void kr::MetalBackend::addPrimitive(ku::Handle<"RenderObject">& handle, krypton::assets::Primitive& primitive,
                                    ku::Handle<"Material">& material) {
    ZoneScoped;
    auto lock = std::scoped_lock(renderObjectMutex);
    auto& object = objects.getFromHandle(handle);
    auto& prim = object.primitives.emplace_back(metal::Primitive { primitive, material, 0, 0, 0 });
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

void kr::MetalBackend::buildMaterial(util::Handle<"Material">& handle) {
    ZoneScoped;
    auto lock = std::scoped_lock(materialMutex);
    VERIFY(materials.isHandleValid(handle));

    auto& material = materials.getFromHandle(handle);

    // According to https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf, a M1, part of
    // the GPUFamilyApple7, can support up to 500.000 buffers and/or textures inside a single
    // ArgumentBuffer.
    materialEncoder->setArgumentBuffer(materialBuffer, 0, handle.getIndex());

    if (material.diffuseTexture.has_value()) {
        auto& diffuseHandle = material.diffuseTexture.value();
        VERIFY(textures.isHandleValid(diffuseHandle));
        materialEncoder->setTexture(textures.getFromHandle(diffuseHandle).texture, 0);
    }
}

void kr::MetalBackend::buildRenderObject(ku::Handle<"RenderObject">& handle) {
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
    if (depthTexture != nullptr)
        depthTexture->release();

    auto depthTexDesc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatDepth32Float_Stencil8, currentRenderTargetDimensions.x,
                                                                    currentRenderTargetDimensions.y, false);
    depthTexDesc->setStorageMode(MTL::StorageModePrivate);
    depthTexDesc->setUsage(MTL::TextureUsageRenderTarget);
    depthTexture = device->newTexture(depthTexDesc);
    depthTexture->setLabel(NS::String::string("Depth Texture", NS::ASCIIStringEncoding));
}

void kr::MetalBackend::createDeferredPipeline() {
    ZoneScoped;
    // Create depth stencil texture
    createDepthTexture();

    // Create textures for our G-Buffer
    createGBufferTextures();

    if (outputColorTexture == nullptr) {
        // Output texture of the FSR pass and input texture for the imgui rendering.
        auto* outputColorTextureDesc = MTL::TextureDescriptor::texture2DDescriptor(colorPixelFormat, currentFramebufferDimensions.x,
                                                                                   currentFramebufferDimensions.y, false);
        outputColorTextureDesc->setStorageMode(MTL::StorageModePrivate);
        outputColorTextureDesc->setUsage(MTL::TextureUsageShaderWrite);
        outputColorTexture = device->newTexture(outputColorTextureDesc);
        outputColorTexture->setLabel(NS::String::string("Output color texture", NS::ASCIIStringEncoding));
        outputColorTextureDesc->release();
    }

    // Create our programs
    auto* vertexProgram = library->newFunction(metal::createString("gbuffer_vertex"));
    auto* fragmentProgram = library->newFunction(metal::createString("gbuffer_fragment"));
    auto* fsqVertexProgram = library->newFunction(metal::createString("fsq_vertex"));
    auto* finalFragmentProgram = library->newFunction(metal::createString("final_fragment"));
    vertexProgram->autorelease();
    fragmentProgram->autorelease();

    // Create the G-Buffer pass PSO
    auto* pipelineStateDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    pipelineStateDescriptor->setVertexFunction(vertexProgram);
    pipelineStateDescriptor->setFragmentFunction(fragmentProgram);
    pipelineStateDescriptor->colorAttachments()->object(0)->setPixelFormat(normalsTexture->pixelFormat());
    pipelineStateDescriptor->colorAttachments()->object(1)->setPixelFormat(albedoTexture->pixelFormat());
    pipelineStateDescriptor->setDepthAttachmentPixelFormat(depthTexture->pixelFormat());

    NS::Error* error = nullptr;
    pipelineState = device->newRenderPipelineState(pipelineStateDescriptor, &error);
    pipelineStateDescriptor->release();
    if (error != nullptr)
        error->release();

    // Create our depth stencil
    auto* depthDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthDescriptor->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionLess);
    depthDescriptor->setDepthWriteEnabled(true);

    depthState = device->newDepthStencilState(depthDescriptor);
    depthDescriptor->release();

    // Create our material buffer. This bases on the encoded length of the argument buffer in our
    // fragment shader, so this buffer is specific to this pipeline and may need to be altered
    // for other pipelines.
    materialEncoder = fragmentProgram->newArgumentEncoder(0);
    materialBuffer = device->newBuffer(maxNumOfMaterials * materialEncoder->encodedLength(), MTL::ResourceStorageModeShared);
    materialBuffer->setLabel(NS::String::string("Material Buffer", NS::ASCIIStringEncoding));

    // Create the color pass PSO
    auto* colorPSDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    colorPSDesc->setVertexFunction(fsqVertexProgram);
    colorPSDesc->setFragmentFunction(finalFragmentProgram);
    colorPSDesc->colorAttachments()->object(0)->setPixelFormat(colorPixelFormat);

    error = nullptr;
    colorPipelineState = device->newRenderPipelineState(colorPSDesc, &error);
    colorPSDesc->release();
}

void kr::MetalBackend::createGBufferTextures() {
    ZoneScoped;

    if (normalsTexture != nullptr)
        normalsTexture->release();
    if (albedoTexture != nullptr)
        albedoTexture->release();
    if (colorTexture != nullptr)
        colorTexture->release();

    auto* normalsDesc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRG11B10Float, currentRenderTargetDimensions.x,
                                                                    currentRenderTargetDimensions.y, false);
    normalsDesc->setStorageMode(MTL::StorageModePrivate);
    normalsDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
    normalsTexture = device->newTexture(normalsDesc);
    normalsTexture->setLabel(NS::String::string("G-Buffer Normals Texture", NS::ASCIIStringEncoding));
    normalsDesc->release();

    auto* albedoDesc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm, currentRenderTargetDimensions.x,
                                                                   currentRenderTargetDimensions.y, false);
    albedoDesc->setStorageMode(MTL::StorageModePrivate);
    albedoDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
    albedoTexture = device->newTexture(albedoDesc);
    albedoTexture->setLabel(NS::String::string("G-Buffer Albedo Texture", NS::ASCIIStringEncoding));
    albedoDesc->release();

    // The texture we write to after our G-Buffer lighting pass. We also use this in the FSR pass.
    auto* colorTextureDesc = MTL::TextureDescriptor::texture2DDescriptor(colorPixelFormat, currentRenderTargetDimensions.x,
                                                                         currentRenderTargetDimensions.y, false);
    colorTextureDesc->setStorageMode(MTL::StorageModePrivate);
    colorTextureDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
    colorTexture = device->newTexture(colorTextureDesc);
    colorTexture->setLabel(NS::String::string("Color texture", NS::ASCIIStringEncoding));
    colorTextureDesc->release();
}

ku::Handle<"RenderObject"> kr::MetalBackend::createRenderObject() {
    ZoneScoped;
    auto mutex = std::scoped_lock<std::mutex>(renderObjectMutex);
    auto refCounter = std::make_shared<ku::ReferenceCounter>();
    return objects.getNewHandle(refCounter);
}

ku::Handle<"Material"> kr::MetalBackend::createMaterial() {
    ZoneScoped;
    auto lock = std::scoped_lock(materialMutex);
    auto refCounter = std::make_shared<ku::ReferenceCounter>();
    auto handle = materials.getNewHandle(refCounter);
    materials.getFromHandle(handle).refCounter = refCounter;
    return handle;
}

ku::Handle<"Texture"> kr::MetalBackend::createTexture() {
    ZoneScoped;
    auto lock = std::scoped_lock(textureMutex);
    auto refCounter = std::make_shared<ku::ReferenceCounter>();
    auto handle = textures.getNewHandle(refCounter);
    textures.getFromHandle(handle).refCounter = refCounter;
    return handle;
}

bool kr::MetalBackend::destroyRenderObject(ku::Handle<"RenderObject">& handle) {
    ZoneScoped;
    auto lock = std::scoped_lock(renderObjectMutex);
    auto valid = objects.isHandleValid(handle);
    if (valid) {
        auto& object = objects.getFromHandle(handle);
        object.vertexBuffer->release();
        object.indexBuffer->release();

        objects.removeHandle(handle);
    } else {
        krypton::log::warn("Tried to destroy a invalid render object handle!");
    }
    return valid;
}

bool kr::MetalBackend::destroyMaterial(ku::Handle<"Material">& handle) {
    ZoneScoped;
    auto lock = std::scoped_lock(materialMutex);
    auto valid = materials.isHandleValid(handle);
    if (valid) {
        auto& mat = materials.getFromHandle(handle);

        if (mat.refCounter->count() == 1) {
            mat.refCounter->decrement();
            materials.removeHandle(handle);
        }
    } else {
        krypton::log::warn("Tried to destroy a invalid material handle!");
    }
    return valid;
}

bool kr::MetalBackend::destroyTexture(util::Handle<"Texture">& handle) {
    ZoneScoped;
    auto lock = std::scoped_lock(textureMutex);
    auto valid = textures.isHandleValid(handle);
    if (valid) {
        auto& tex = textures.getFromHandle(handle);

        if (tex.refCounter->count() == 1) {
            tex.refCounter->decrement();
            tex.texture->release();
            textures.removeHandle(handle);
        }
    } else {
        krypton::log::warn("Tried to destroy a invalid texture handle!");
    }
    return valid;
}

void kr::MetalBackend::drawFrame() {
    ZoneScoped;

    // Render our own ImGui window for backend configuration
    if (ImGui::Begin("Metal backend")) {
        ImGui::Text("%s", fmt::format("Rendering on {}", device->name()->utf8String()).c_str());

        if (ImGui::BeginCombo("AMD FSR 1.0 Profile", selectedProfile.name.c_str())) {
            auto profiles = kr::getFsrProfiles();
            for (auto& item : profiles) {
                if (ImGui::Selectable(item.name.c_str(), item.factor == selectedProfile.factor)) {
                    selectedProfile = item;
                    currentRenderTargetDimensions = getScaledResolution(currentFramebufferDimensions, selectedProfile);

                    // Re-create the render targets
                    createDepthTexture();
                    createGBufferTextures();
                    updateFSRBuffers();
                }
            }

            ImGui::EndCombo();
        }
        ImGui::End();
    }

    auto* arPool = NS::AutoreleasePool::alloc()->init();
    auto* drawable = swapchain->nextDrawable();

    // Create render pass
    auto* mainPass = MTL::RenderPassDescriptor::renderPassDescriptor();

    auto* normalsAttachment = mainPass->colorAttachments()->object(0);
    normalsAttachment->setClearColor(MTL::ClearColor::Make(0, 0, 0, 0));
    normalsAttachment->setLoadAction(MTL::LoadActionClear);
    normalsAttachment->setStoreAction(MTL::StoreActionStore);
    normalsAttachment->setTexture(normalsTexture);

    auto* albedoAttachment = mainPass->colorAttachments()->object(1);
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
    encoder->setFragmentBuffer(materialBuffer, 0, 0);

    for (const auto& handle : handlesForFrame) {
        const auto& object = objects.getFromHandle(handle);

        encoder->setVertexBuffer(object.vertexBuffer, 0, 0); /* Vertex buffer is at 0 */
        encoder->setVertexBuffer(object.instanceBuffer, 0, 1);

        for (const auto& primitive : object.primitives) {
            VERIFY(primitive.material.has_value());

            encoder->setFragmentBufferOffset(primitive.material->getIndex() * materialEncoder->encodedLength(), 0);

            // We need these two lines so that Metal knows that the texture is now being used.
            // Otherwise, we might encounter a page fault or something similar. I know, this is
            // hideous code.
            auto& diffuseTexture = materials.getFromHandle(primitive.material.value()).diffuseTexture.value();
            encoder->useResource(textures.getFromHandle(diffuseTexture).texture, MTL::ResourceUsageSample, MTL::RenderStageFragment);

            /* Dispatch draw calls */
            encoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, primitive.indexCount, MTL::IndexTypeUInt32, object.indexBuffer,
                                           primitive.indexBufferOffset, 1, primitive.baseVertex, 0);
        }
    }

    encoder->endEncoding();

    // Now the color pass, using the two textures produced by the G-Buffer pass.
    auto* colorPass = MTL::RenderPassDescriptor::renderPassDescriptor();
    auto* colorAttachment = colorPass->colorAttachments()->object(0);
    colorAttachment->setClearColor(MTL::ClearColor::Make(0, 0, 0, 0.25));
    colorAttachment->setLoadAction(MTL::LoadActionClear);
    colorAttachment->setStoreAction(MTL::StoreActionStore);
    colorAttachment->setTexture(colorTexture);

    auto* colorEncoder = commandBuffer->renderCommandEncoder(colorPass);
    colorEncoder->setRenderPipelineState(colorPipelineState);
    colorEncoder->setFragmentTexture(normalsTexture, 0);
    colorEncoder->setFragmentTexture(albedoTexture, 1);

    colorEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, (NS::UInteger)0, 3);
    colorEncoder->endEncoding();

    // Now our FSR pass
    auto* fsrPass = MTL::ComputePassDescriptor::computePassDescriptor();
    fsrPass->setDispatchType(MTL::DispatchTypeConcurrent);
    auto* fsrEncoder = commandBuffer->computeCommandEncoder(fsrPass);

    static const int threadGroupWorkRegionDim = 16;
    uint32_t fsrDispatchX = (currentFramebufferDimensions.x + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
    uint32_t fsrDispatchY = (currentFramebufferDimensions.y + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
    auto fsrDispatchSize = MTL::Size::Make(fsrDispatchX, fsrDispatchY, 1);

    // We expect 64, 1, 1. While this is probably not the most optimal thing, this is required and
    // will usually fulfill the rule of threadExecutionWidth() * x.
    MTL::Size threadsPerThreadgroup = MTL::Size::Make(64, 1, 1);

    fsrEncoder->setComputePipelineState(fsrComputePSO);
    fsrEncoder->setBuffer(fsrArgumentBuffer, 0, 0);
    fsrEncoder->useResource(colorTexture, MTL::ResourceUsageSample | MTL::ResourceUsageRead);
    fsrEncoder->useResource(outputColorTexture, MTL::ResourceUsageWrite);
    fsrEncoder->useResource(fsrEasuBuffer, MTL::ResourceUsageRead);
    fsrEncoder->dispatchThreadgroups(fsrDispatchSize, threadsPerThreadgroup);
    fsrEncoder->endEncoding();

    // We now need to copy our outputColorTexture to the drawable. I really hate this, and I hope
    // there's a better way to do this.
    auto* blitEncoder = commandBuffer->blitCommandEncoder();
    blitEncoder->copyFromTexture(outputColorTexture, 0, 0, MTL::Origin::Make(0, 0, 0),
                                 MTL::Size::Make(currentFramebufferDimensions.x, currentFramebufferDimensions.y, 1), drawable->texture(), 0,
                                 0, MTL::Origin::Make(0, 0, 0));
    blitEncoder->endEncoding();

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

    colorPixelFormat = static_cast<MTL::PixelFormat>(metal::getScreenPixelFormat(window->getWindowPointer()));

    int w_width, w_height;
    window->getWindowSize(&w_width, &w_height);
    krypton::log::log("Window size: {}x{}", w_width, w_height);

    int width, height;
    window->getFramebufferSize(&width, &height);
    currentFramebufferDimensions = { width, height };
    krypton::log::log("Framebuffer size: {}x{}", width, height);

    // We have to set this because the window otherwise defaults to our
    // scaled size and not the actual wanted framebuffer size.
    CG::Size drawableSize = {
        .width = (double)width,
        .height = (double)height,
    };

    // Create the device, queue and metal layer
    device = MTL::CreateSystemDefaultDevice();
    if (device->argumentBuffersSupport() < MTL::ArgumentBuffersTier2) {
        krypton::log::throwError("The Metal backend requires support at least ArgumentBuffersTier2.");
        return;
    }

    queue = device->newCommandQueue();
    swapchain = CA::MetalLayer::layer();
    swapchain->setDevice(device);
    swapchain->setPixelFormat(colorPixelFormat);
    swapchain->setDrawableSize(drawableSize);
    swapchain->setFramebufferOnly(false);

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
    if (error != nullptr)
        error->release();

    initFSR();
    currentRenderTargetDimensions = getScaledResolution(currentFramebufferDimensions, selectedProfile);

    krypton::log::log("Scale factor: {}", selectedProfile.factor);
    krypton::log::log("Output resolution: {}x{}", width, height);
    krypton::log::log("Scaled resolution: {}x{}", currentRenderTargetDimensions.x, currentRenderTargetDimensions.y);

    createDeferredPipeline();

    if (cameraData == nullptr)
        cameraData = std::make_shared<kr::CameraData>();

    cameraBuffer = device->newBuffer(kr::CAMERA_DATA_SIZE, MTL::ResourceStorageModeShared);

    imGuiPassDescriptor = MTL::RenderPassDescriptor::renderPassDescriptor();

    updateFSRBuffers();
}

void kr::MetalBackend::initFSR() {
    auto fsrShader = krypton::shaders::readShaderFile("shaders/fsr.metal");
    NS::String* fsrShaderSource = NS::String::string(fsrShader.content.c_str(), NS::ASCIIStringEncoding);

    auto* compileOptions = MTL::CompileOptions::alloc()->init();
    compileOptions->setLanguageVersion(MTL::LanguageVersion2_4);

    NS::Error* error = nullptr;
    fsrLibrary = device->newLibrary(fsrShaderSource, compileOptions, &error);
    if (!fsrLibrary) {
        krypton::log::err(error->description()->utf8String());
    }
    if (error != nullptr)
        error->release();

    fsrComputeFunction = fsrLibrary->newFunction(metal::createString("main0"));
    error = nullptr;
    fsrComputePSO = device->newComputePipelineState(fsrComputeFunction, &error);
    if (!fsrComputePSO) {
        krypton::log::err(error->description()->utf8String());
    }
    if (error != nullptr)
        error->release();

    if (fsrArgumentEncoder == nullptr) {
        fsrArgumentEncoder = fsrComputeFunction->newArgumentEncoder(0);
        fsrArgumentBuffer = device->newBuffer(fsrArgumentEncoder->encodedLength(), MTL::ResourceStorageModeShared);
        fsrArgumentBuffer->setLabel(NS::String::string("FSR Argument buffer", NS::ASCIIStringEncoding));
    }

    std::vector<uint32_t> easu;
    configureEasu(easu, currentRenderTargetDimensions, currentFramebufferDimensions);
    if (fsrEasuBuffer == nullptr) {
        fsrEasuBuffer = device->newBuffer(easu.size() * sizeof(uint32_t), MTL::ResourceStorageModeShared);
        fsrEasuBuffer->setLabel(NS::String::string("FSR Constants buffer", NS::ASCIIStringEncoding));
    }
    std::memcpy(fsrEasuBuffer->contents(), easu.data(), fsrEasuBuffer->length());
}

void kr::MetalBackend::render(ku::Handle<"RenderObject"> handle) {
    ZoneScoped;
    if (objects.isHandleValid(handle))
        handlesForFrame.push_back(handle);
}

void kr::MetalBackend::resize(int width, int height) {
    ZoneScoped;
    // Metal auto-resizes for us. Though we still have to re-recreate the deferred pipeline.
    depthTexture->release();
    normalsTexture->release();
    albedoTexture->release();

    createDeferredPipeline();
}

void kr::MetalBackend::setMaterialBaseColor(util::Handle<"Material">& handle, glm::fvec4 baseColor) {
    // todo
}

void kr::MetalBackend::setMaterialDiffuseTexture(util::Handle<"Material"> handle, util::Handle<"Texture"> textureHandle) {
    ZoneScoped;
    auto lock = std::scoped_lock(materialMutex);

    materials.getFromHandle(handle).diffuseTexture = std::move(textureHandle);
}

void kr::MetalBackend::setObjectName(ku::Handle<"RenderObject">& handle, std::string name) {}

void kr::MetalBackend::setObjectTransform(ku::Handle<"RenderObject">& handle, glm::mat4x3 transform) {
    ZoneScoped;
    auto lock = std::scoped_lock(renderObjectMutex);
    objects.getFromHandle(handle).transform = transform;
}

void kr::MetalBackend::setTextureColorEncoding(util::Handle<"Texture"> handle, kr::ColorEncoding colorEncoding) {
    auto lock = std::scoped_lock(textureMutex);
    textures.getFromHandle(handle).encoding = colorEncoding;
}

void kr::MetalBackend::setTextureData(const util::Handle<"Texture">& handle, uint32_t width, uint32_t height, std::span<std::byte> pixels,
                                      kr::TextureFormat format) {
    ZoneScoped;
    auto lock = std::scoped_lock(textureMutex);
    auto& texture = textures.getFromHandle(handle);
    texture.width = width;
    texture.height = height;
    texture.format = format;
    texture.textureData.resize(pixels.size_bytes());
    std::memcpy(texture.textureData.data(), pixels.data(), pixels.size_bytes());
}

void kr::MetalBackend::setTextureName(util::Handle<"Texture"> handle, std::string name) {
    auto lock = std::scoped_lock(textureMutex);
    auto& tex = textures.getFromHandle(handle);
    tex.name = name;
    if (tex.texture != nullptr)
        tex.texture->setLabel(NS::String::string(name.data(), NS::UTF8StringEncoding));
}

void kr::MetalBackend::shutdown() {
    ZoneScoped;
    albedoTexture->release();
    normalsTexture->release();
    depthTexture->release();

    colorPipelineState->release();
    pipelineState->release();

    imGuiPassDescriptor->release();
    ImGui_ImplMetal_DestroyDeviceObjects();
    queue->release();
    library->release();
    device->release();
}

void kr::MetalBackend::triggerCapture() {
    bool success;

    MTL::CaptureManager* pCaptureManager = MTL::CaptureManager::sharedCaptureManager();
    success = pCaptureManager->supportsDestination(MTL::CaptureDestinationGPUTraceDocument);
    if (!success) {
        krypton::log::throwError("Capture support is not enabled");
    }

    char filename[NAME_MAX];
    std::time_t now;
    std::time(&now);
    std::strftime(filename, NAME_MAX, "capture-%H-%M-%S_%m-%d-%y.gputrace", std::localtime(&now));

    NS::URL* pURL = NS::URL::alloc()->initFileURLWithPath(NS::String::string(filename, NS::ASCIIStringEncoding));

    MTL::CaptureDescriptor* pCaptureDescriptor = MTL::CaptureDescriptor::alloc()->init();
    pCaptureDescriptor->setDestination(MTL::CaptureDestinationGPUTraceDocument);
    pCaptureDescriptor->setOutputURL(pURL);
    pCaptureDescriptor->setCaptureObject(device);

    NS::Error* pError = nullptr;
    success = pCaptureManager->startCapture(pCaptureDescriptor, &pError);
    if (!success) {
        krypton::log::throwError(R"(Failed to start capture: "{}" for file "{}")", pError->localizedDescription()->utf8String(), filename);
    }

    pURL->release();
    pCaptureDescriptor->release();
}

void kr::MetalBackend::updateFSRBuffers() {
    VERIFY(fsrEasuBuffer != nullptr);

    fsrArgumentEncoder->setArgumentBuffer(fsrArgumentBuffer, 0, 0);
    fsrArgumentEncoder->setTexture(colorTexture, 0);
    fsrArgumentEncoder->setBuffer(fsrEasuBuffer, 0, 1);
    fsrArgumentEncoder->setTexture(outputColorTexture, 2);

    std::vector<uint32_t> easu;
    configureEasu(easu, currentRenderTargetDimensions, currentFramebufferDimensions);
    std::memcpy(fsrEasuBuffer->contents(), easu.data(), fsrEasuBuffer->length());
}

void kr::MetalBackend::uploadTexture(util::Handle<"Texture"> handle) {
    ZoneScoped;
    auto lock = std::scoped_lock(textureMutex);
    auto& texture = textures.getFromHandle(handle);

    auto* texDesc = MTL::TextureDescriptor::texture2DDescriptor(metal::getPixelFormat(texture.format, texture.encoding), texture.width,
                                                                texture.height, false);
    texDesc->setUsage(MTL::TextureUsageShaderRead);
    texDesc->setStorageMode(MTL::StorageModeShared);
    auto* mtlTexture = device->newTexture(texDesc);
    mtlTexture->setLabel(NS::String::string(texture.name.c_str(), NS::UTF8StringEncoding));

    // Upload the texture data into a buffer to upload to the texture.
    MTL::Region imageRegion = MTL::Region::Make2D(0, 0, texture.width, texture.height);
    mtlTexture->replaceRegion(imageRegion, 0, texture.textureData.data(), 4 * texture.width);

    texture.texture = mtlTexture;
}

#endif // #ifdef RAPI_WITH_METAL
