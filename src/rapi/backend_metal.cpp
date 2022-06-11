#ifdef RAPI_WITH_METAL

#include <algorithm>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <Tracy.hpp>

#include <rapi/backend_metal.hpp>
#include <rapi/metal/glfw_cocoa_bridge.hpp>
#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_sampler.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/shader_types.hpp>
#include <util/large_free_list.hpp>
#include <util/logging.hpp>

namespace krypton::rapi::metal {
    MTL::VertexFormat getVertexFormat(VertexFormat vertexFormat) {
        switch (vertexFormat) {
            case VertexFormat::RGBA32_FLOAT: {
                return MTL::VertexFormatFloat4;
            }
            case VertexFormat::RGB32_FLOAT: {
                return MTL::VertexFormatFloat3;
            }
            case VertexFormat::RG32_FLOAT: {
                return MTL::VertexFormatFloat2;
            }
            case VertexFormat::RGBA8_UNORM: {
                return MTL::VertexFormatUChar4;
            }
        }

        return MTL::VertexFormatInvalid;
    }
} // namespace krypton::rapi::metal

namespace kr = krypton::rapi;
namespace ku = krypton::util;

#pragma region MetalBackend
kr::MetalBackend::MetalBackend() {
    backendAutoreleasePool = NS::AutoreleasePool::alloc()->init();
    window = std::make_shared<kr::Window>("Krypton", 1920, 1080);
}

kr::MetalBackend::~MetalBackend() noexcept {
    // Any ObjC objects allocated from an object from the MetalBackend will be cleaned up by this.
    // If the user should continue using any RAPI objects allocated for this backend, they will
    // break and likely cause some sort of crash. To release objects manually, call retain() on
    // it and then release(). Otherwise, this call will trigger an exception. Also note that this
    // only affects the current thread, meaning calls to RAPI objects from other threads will have
    // to be released manually, which is generally recommended anyway.
    backendAutoreleasePool->drain();
}

void kr::MetalBackend::addRenderPassAttachment(const util::Handle<"RenderPass">& handle, uint32_t index, RenderPassAttachment attachment) {
    ZoneScoped;
    auto& pass = renderPasses.getFromHandle(handle);
    pass.attachments[index] = attachment;
}

void kr::MetalBackend::beginFrame() {
    ZoneScoped;
    window->newFrame();

    // Every ObjC resource created within the frame with a function that does not begin with 'new',
    // or 'alloc', will automatically be freed at the end of the frame if retain() was not called.
    frameAutoreleasePool = NS::AutoreleasePool::alloc()->init();
}

void kr::MetalBackend::buildRenderPass(util::Handle<"RenderPass">& handle) {
    auto& pass = renderPasses.getFromHandle(handle);

    auto* psoDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    auto* depthDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    pass.descriptor = MTL::RenderPassDescriptor::alloc()->init();

    // Set pipeline functions
    psoDescriptor->setVertexFunction(pass.vertexFunction);
    psoDescriptor->setFragmentFunction(pass.fragmentFunction);

    auto* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

    // Setup vertex buffer layouts
    for (auto i = 0ULL; i < pass.vertexDescriptor.buffers.size(); ++i) {
        auto* layout = vertexDescriptor->layouts()->object(i);
        layout->setStride(pass.vertexDescriptor.buffers[i].stride);
        switch (pass.vertexDescriptor.buffers[i].inputRate) {
            case VertexInputRate::Vertex: {
                layout->setStepFunction(MTL::VertexStepFunctionPerVertex);
                break;
            }
            case VertexInputRate::Instance: {
                layout->setStepFunction(MTL::VertexStepFunctionPerInstance);
                break;
            }
        }
    }

    // Setup vertex attributes
    for (auto i = 0ULL; i < pass.vertexDescriptor.attributes.size(); ++i) {
        auto* attrib = vertexDescriptor->attributes()->object(i);
        attrib->setBufferIndex(pass.vertexDescriptor.attributes[i].bufferIndex);
        attrib->setOffset(pass.vertexDescriptor.attributes[i].offset);
        attrib->setFormat(metal::getVertexFormat(pass.vertexDescriptor.attributes[i].format));
    }

    psoDescriptor->setVertexDescriptor(vertexDescriptor);

    // Load attachments
    for (auto& pair : pass.attachments) {
        auto& cc = pair.second.clearColor;

        // We had to make the attachment member a std::optional to avoid having to call a Handle's
        // default ctor which doesn't exist.
        auto* rAttachment = pass.descriptor->colorAttachments()->object(pair.first);
        rAttachment->init();
        rAttachment->setLoadAction(metal::loadActions[pair.second.loadAction]);
        rAttachment->setStoreAction(metal::storeActions[pair.second.storeAction]);
        rAttachment->setClearColor(MTL::ClearColor::Make(cc.r, cc.g, cc.b, cc.a));
        // We don't bind textures here yet.

        auto* pAttachment = psoDescriptor->colorAttachments()->object(pair.first);
        pAttachment->init();
        if (pair.second.attachment.get() == renderTargetTexture.get()) {
            pAttachment->setPixelFormat(colorPixelFormat);
        } else {
            pAttachment->setPixelFormat(dynamic_cast<metal::Texture*>(pair.second.attachment.get())->texture->pixelFormat());
        }
    }

    if (pass.depthAttachment.has_value()) {
        auto depthTex = dynamic_cast<metal::Texture*>(pass.depthAttachment->attachment.get());

        auto depth = pass.descriptor->depthAttachment();
        depth->setClearDepth(pass.depthAttachment->clearDepth);
        depth->setLoadAction(metal::loadActions[pass.depthAttachment->loadAction]);
        depth->setStoreAction(metal::storeActions[pass.depthAttachment->storeAction]);
        depth->setTexture(depthTex->texture);

        psoDescriptor->setDepthAttachmentPixelFormat(depthTex->texture->pixelFormat());

        depthDescriptor->init();
        depthDescriptor->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionLess);
        depthDescriptor->setDepthWriteEnabled(true);
    }

    NS::Error* error = nullptr;
    pass.pipelineState = device->newRenderPipelineState(psoDescriptor, &error);
    if (!pass.pipelineState) {
        krypton::log::err(error->localizedDescription()->utf8String());
    }
    psoDescriptor->release();
    vertexDescriptor->release();

    pass.depthState = device->newDepthStencilState(depthDescriptor);
    depthDescriptor->release();
}

std::shared_ptr<kr::IBuffer> kr::MetalBackend::createBuffer() {
    return std::make_shared<metal::Buffer>(device);
}

ku::Handle<"RenderPass"> kr::MetalBackend::createRenderPass() {
    ZoneScoped;
    auto refCounter = std::make_shared<ku::ReferenceCounter>();
    return renderPasses.getNewHandle(std::move(refCounter));
}

std::shared_ptr<kr::ISampler> kr::MetalBackend::createSampler() {
    ZoneScoped;
    return std::make_shared<kr::metal::Sampler>(device);
}

std::shared_ptr<kr::IShader> kr::MetalBackend::createShaderFunction(std::span<const std::byte> bytes,
                                                                    krypton::shaders::ShaderSourceType type,
                                                                    krypton::shaders::ShaderStage stage) {
    ZoneScoped;
    switch (stage) {
        case krypton::shaders::ShaderStage::Fragment: {
            return std::make_shared<metal::FragmentShader>(device, bytes, type);
        }
        case krypton::shaders::ShaderStage::Vertex: {
            return std::make_shared<metal::VertexShader>(device, bytes, type);
        }
        default:
            krypton::log::throwError("No support for given shader stage: {}", static_cast<uint32_t>(stage));
    }
}

std::shared_ptr<kr::IShaderParameter> kr::MetalBackend::createShaderParameter() {
    ZoneScoped;
    return std::make_shared<metal::ShaderParameter>(device);
}

std::shared_ptr<kr::ITexture> kr::MetalBackend::createTexture(rapi::TextureUsage usage) {
    ZoneScoped;
    return std::make_shared<metal::Texture>(device, usage);
}

bool kr::MetalBackend::destroyRenderPass(util::Handle<"RenderPass">& handle) {
    auto valid = renderPasses.isHandleValid(handle);
    if (valid) {
        auto& pass = renderPasses.getFromHandle(handle);
        pass.descriptor->release();
        pass.pipelineState->release();
        pass.depthState->release();
        renderPasses.removeHandle(handle); // Calls destructor.
    } else {
        krypton::log::warn("Tried to destroy a invalid render pass handle");
    }
    return valid;
}

void kr::MetalBackend::endFrame() {
    ZoneScoped;
    // Drains any resources created within the render loop. Also releases the pool itself.
    frameAutoreleasePool->drain();
}

std::unique_ptr<kr::ICommandBuffer> kr::MetalBackend::getFrameCommandBuffer() {
    ZoneScoped;
    auto cmdBuffer = std::make_unique<kr::metal::CommandBuffer>();
    cmdBuffer->rapi = std::dynamic_pointer_cast<kr::MetalBackend>(getPointer());
    cmdBuffer->drawable = swapchain->nextDrawable();
    cmdBuffer->buffer = queue->commandBuffer();

    dynamic_cast<metal::Texture*>(renderTargetTexture.get())->texture = cmdBuffer->drawable->texture();

    return std::move(cmdBuffer);
}

std::shared_ptr<kr::ITexture> kr::MetalBackend::getRenderTargetTextureHandle() {
    return renderTargetTexture;
}

std::shared_ptr<kr::Window> kr::MetalBackend::getWindow() {
    return window;
}

void kr::MetalBackend::init() {
    ZoneScopedN("MetalBackend::init");
    window->create(kr::Backend::Metal);
    window->setRapiPointer(static_cast<krypton::rapi::RenderAPI*>(this));

    colorPixelFormat = static_cast<MTL::PixelFormat>(metal::getScreenPixelFormat(window->getWindowPointer()));

    auto wSize = window->getWindowSize();
    krypton::log::log("Window size: {}x{}", wSize.x, wSize.y);

    auto fbSize = window->getFramebufferSize();
    krypton::log::log("Framebuffer size: {}x{}", fbSize.x, fbSize.y);

    // Create the device, queue and metal layer
    device = MTL::CreateSystemDefaultDevice();
    device->autorelease();
    if (device->argumentBuffersSupport() < MTL::ArgumentBuffersTier2) {
        krypton::log::throwError("The Metal backend requires support at least ArgumentBuffersTier2.");
    }

    queue = device->newCommandQueue();
    queue->autorelease(); // Makes the Queue be released automatically by backendAutoreleasePool
    swapchain = window->createMetalLayer(device, colorPixelFormat);

    krypton::log::log("Setting up Metal on {}", device->name()->utf8String());

    // Create our Metal library
    defaultShader = krypton::shaders::readShaderFile("shaders/shaders.metal");
    const NS::String* shaderSource = NS::String::string(defaultShader.content.c_str(), NS::ASCIIStringEncoding);

    auto* compileOptions = MTL::CompileOptions::alloc()->init();
    compileOptions->setLanguageVersion(MTL::LanguageVersion2_4);

    NS::Error* error = nullptr;
    library = device->newLibrary(shaderSource, compileOptions, &error);
    if (!library) {
        krypton::log::err(error->description()->utf8String());
    }
    library->autorelease();

    renderTargetTexture = createTexture(rapi::TextureUsage::ColorRenderTarget | rapi::TextureUsage::SampledImage);
}

void kr::MetalBackend::resize(int width, int height) {
    ZoneScoped;
}

void kr::MetalBackend::setRenderPassFragmentFunction(const util::Handle<"RenderPass">& handle, const IShader* const shader) {
    auto& rp = renderPasses.getFromHandle(handle);
    rp.fragmentFunction = dynamic_cast<const metal::FragmentShader*>(shader)->function;
}

void kr::MetalBackend::setRenderPassVertexDescriptor(const util::Handle<"RenderPass">& handle, VertexDescriptor descriptor) {
    renderPasses.getFromHandle(handle).vertexDescriptor = descriptor;
}

void kr::MetalBackend::setRenderPassVertexFunction(const util::Handle<"RenderPass">& handle, const IShader* const shader) {
    auto& rp = renderPasses.getFromHandle(handle);
    rp.vertexFunction = dynamic_cast<const metal::VertexShader*>(shader)->function;
}

void kr::MetalBackend::shutdown() {
    ZoneScoped;
}
#pragma endregion

#endif // #ifdef RAPI_WITH_METAL
