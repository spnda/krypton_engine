#include <utility>

#include <Tracy.hpp>
#include <fmt/format.h>
#include <imgui_impl_glfw.h>

#include <core/imgui_renderer.hpp>
#include <rapi/icommandbuffer.hpp>
#include <rapi/iswapchain.hpp>
#include <rapi/render_pass_attachments.hpp>
#include <rapi/vertex_descriptor.hpp>
#include <rapi/window.hpp>
#include <shaders/shaders.hpp>

namespace kc = krypton::core;

kc::ImGuiRenderer::ImGuiRenderer(rapi::IDevice* device, krypton::rapi::Window* window) : uniforms({}), window(window), device(device) {}

void kc::ImGuiRenderer::buildFontTexture(ImGuiIO& io) {
    ZoneScoped;
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    fontAtlas = device->createTexture(rapi::TextureUsage::SampledImage);
    fontAtlas->setName("ImGui font texture");
    fontAtlas->setSwizzling(rapi::SwizzleChannels {
        .r = rapi::TextureSwizzle::One,
        .g = rapi::TextureSwizzle::One,
        .b = rapi::TextureSwizzle::One,
        .a = rapi::TextureSwizzle::Red,
    });
    fontAtlas->create(rapi::TextureFormat::R8_SRGB, width, height);
    fontAtlas->uploadTexture(std::span { reinterpret_cast<std::byte*>(pixels), static_cast<std::size_t>(width * height) });

    if (fontAtlasSampler == nullptr) {
        // This likely won't change.
        fontAtlasSampler = device->createSampler();
        fontAtlasSampler->setName("ImGui font-atlas sampler");
        fontAtlasSampler->createSampler();
    }

    io.Fonts->SetTexID(static_cast<ImTextureID>(fontAtlas.get()));
}

void kc::ImGuiRenderer::init(rapi::ISwapchain* pSwapchain) {
    ZoneScopedN("ImGuiRenderer::init");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOther(window->getWindowPointer(), true);

    swapchain = pSwapchain;

    auto& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendRendererName = "krypton::core::ImGuiRenderer";
    io.BackendPlatformName = "krypton::rapi::RenderAPI";

    buildFontTexture(io);

    // Create the index/vertex buffers. As the swapchain implementation might have multiple
    // swapchain images, meaning we have multiple frames in flight, we'll need unique buffers
    // for each frame in flight to avoid any race conditions.
    buffers.resize(swapchain->getImageCount());
    for (auto i = 0UL; i < swapchain->getImageCount(); ++i) {
        buffers[i].vertexBuffer = device->createBuffer();
        buffers[i].indexBuffer = device->createBuffer();

        buffers[i].vertexBuffer->setName(fmt::format("ImGui Vertex Buffer {}", i));
        buffers[i].indexBuffer->setName(fmt::format("ImGui Index Buffer {}", i));

        buffers[i].vertexBuffer->create(sizeof(ImDrawVert), rapi::BufferUsage::UniformBuffer | rapi::BufferUsage::VertexBuffer,
                                        rapi::BufferMemoryLocation::SHARED);
        buffers[i].indexBuffer->create(sizeof(ImDrawIdx), rapi::BufferUsage::UniformBuffer | rapi::BufferUsage::IndexBuffer,
                                       rapi::BufferMemoryLocation::SHARED);

        // Build our uniform buffer
        buffers[i].uniformBuffer = device->createBuffer();
        buffers[i].uniformBuffer->setName(fmt::format("ImGui Uniform Buffer {}", i + 1));
        buffers[i].uniformBuffer->create(sizeof(ImGuiShaderUniforms), rapi::BufferUsage::UniformBuffer, rapi::BufferMemoryLocation::SHARED);

        updateUniformBuffer(buffers[i].uniformBuffer.get(), io.DisplaySize, ImVec2(0, 0));

        buffers[i].uniformShaderParameter = device->createShaderParameter();
        buffers[i].uniformShaderParameter->addBuffer(0, buffers[i].uniformBuffer);
        buffers[i].uniformShaderParameter->buildParameter();
    }

    // Create the shaders
    auto fragGlsl = krypton::shaders::readShaderFile("shaders/ui.frag");
    auto vertGlsl = krypton::shaders::readShaderFile("shaders/ui.vert");

    fragmentShader =
        device->createShaderFunction({ reinterpret_cast<const std::byte*>(fragGlsl.content.data()), fragGlsl.content.size() + 1 },
                                     krypton::shaders::ShaderSourceType::GLSL, krypton::shaders::ShaderStage::Fragment);
    vertexShader =
        device->createShaderFunction({ reinterpret_cast<const std::byte*>(vertGlsl.content.data()), vertGlsl.content.size() + 1 },
                                     krypton::shaders::ShaderSourceType::GLSL, krypton::shaders::ShaderStage::Vertex);

    if (fragmentShader->needsTranspile())
        fragmentShader->transpile("main", krypton::shaders::ShaderStage::Fragment);
    if (vertexShader->needsTranspile())
        vertexShader->transpile("main", krypton::shaders::ShaderStage::Vertex);

    fragmentShader->createModule();
    vertexShader->createModule();

    // Create the pipeline
    pipeline = device->createPipeline();
    pipeline->setFragmentFunction(fragmentShader.get());
    pipeline->setVertexFunction(vertexShader.get());
    // clang-format off
    pipeline->setVertexDescriptor({
        .buffers = {
            {
                .stride = sizeof(ImDrawVert),
                .inputRate = rapi::VertexInputRate::Vertex,
            },
        },
        .attributes = {
            {
                .offset = static_cast<uint32_t>(offsetof(ImDrawVert, pos)),
                .bufferIndex = 0,
                .format = rapi::VertexFormat::RG32_FLOAT,
            },
            {
                .offset = static_cast<uint32_t>(offsetof(ImDrawVert, uv)),
                .bufferIndex = 0,
                .format = rapi::VertexFormat::RG32_FLOAT,
            },
            {
                .offset = static_cast<uint32_t>(offsetof(ImDrawVert, col)),
                .bufferIndex = 0,
                .format = rapi::VertexFormat::RGBA8_UNORM,
            },
        }
    });
    pipeline->addAttachment(0, {
        .format = swapchain->getDrawableFormat(),
        .blending = {
            .enabled = true,
            .rgbOperation = rapi::BlendOperation::Add,
            .alphaOperation = rapi::BlendOperation::Add,

            .rgbSourceFactor = rapi::BlendFactor::SourceAlpha,
            .rgbDestinationFactor = rapi::BlendFactor::OneMinusSourceAlpha,
            .alphaSourceFactor = rapi::BlendFactor::One,
            .alphaDestinationFactor = rapi::BlendFactor::OneMinusSourceAlpha,
        }
    });
    // clang-format on
    pipeline->create();

    // Create the render pass
    renderPass = device->createRenderPass();
    // clang-format off
    renderPass->setAttachment(0, {
        .attachment = swapchain->getDrawable(),
        .attachmentFormat = swapchain->getDrawableFormat(),
        .loadAction = krypton::rapi::AttachmentLoadAction::Load,
        .storeAction = krypton::rapi::AttachmentStoreAction::Store,
        .clearColor = glm::fvec4(0.0),
    });
    // clang-format on
    renderPass->build();

    // Build our font atlas shader parameter
    textureShaderParameter = device->createShaderParameter();
    textureShaderParameter->addTexture(0, fontAtlas);
    textureShaderParameter->addSampler(1, fontAtlasSampler);
    textureShaderParameter->buildParameter();
}

void kc::ImGuiRenderer::destroy() {
    for (auto& buf : buffers) {
        buf.indexBuffer->destroy();
        buf.vertexBuffer->destroy();
        buf.uniformShaderParameter->destroy();
        buf.uniformBuffer->destroy();
    }

    textureShaderParameter->destroy();
    fontAtlasSampler->destroy();
    fontAtlas->destroy();

    pipeline->destroy();
    renderPass->destroy();

    fragmentShader->destroy();
    vertexShader->destroy();

    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void kc::ImGuiRenderer::draw(rapi::ICommandBuffer* commandBuffer) {
    ZoneScoped;
    ImGui::Render();
    auto* drawData = ImGui::GetDrawData();

    currentFrame = ++currentFrame % swapchain->getImageCount();

    auto& vertexBuffer = buffers[currentFrame].vertexBuffer;
    auto& indexBuffer = buffers[currentFrame].indexBuffer;

    // Copy all vertex and index buffers into the proper buffers. Because of Vulkan, we cannot copy
    // buffers while within a render pass.
    if (drawData->TotalVtxCount > 0) {
        auto commandLists = std::span(drawData->CmdLists, drawData->CmdListsCount);

        // Update the uniform buffer
        updateUniformBuffer(buffers[currentFrame].uniformBuffer.get(), drawData->DisplaySize, drawData->DisplayPos);

        size_t vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
        size_t indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

        // We will have to resize the buffers if they're not large enough for all the data.
        if (vertexBufferSize > vertexBuffer->getSize()) {
            vertexBuffer->destroy();
            vertexBuffer->create(vertexBufferSize, rapi::BufferUsage::UniformBuffer | rapi::BufferUsage::VertexBuffer,
                                 rapi::BufferMemoryLocation::SHARED);
        }

        if (indexBufferSize > indexBuffer->getSize()) {
            indexBuffer->destroy();
            indexBuffer->create(vertexBufferSize, rapi::BufferUsage::UniformBuffer | rapi::BufferUsage::IndexBuffer,
                                rapi::BufferMemoryLocation::SHARED);
        }

        // Copy the vertex and index buffers
        {
            void* vtxData = nullptr;
            void* idxData = nullptr;
            vertexBuffer->mapMemory(&vtxData);
            indexBuffer->mapMemory(&idxData);

            auto* vertexDestination = static_cast<ImDrawVert*>(vtxData);
            auto* indexDestination = static_cast<ImDrawIdx*>(idxData);
            for (auto& list : commandLists) {
                std::memcpy(vertexDestination, list->VtxBuffer.Data, list->VtxBuffer.Size * sizeof(ImDrawVert));
                std::memcpy(indexDestination, list->IdxBuffer.Data, list->IdxBuffer.Size * sizeof(ImDrawIdx));

                // Because the destination pointers have a type of ImDrawXYZ*, it already
                // properly takes the byte size into account.
                vertexDestination += list->VtxBuffer.Size;
                indexDestination += list->IdxBuffer.Size;
            }

            vertexBuffer->unmapMemory();
            indexBuffer->unmapMemory();
        }

        (*renderPass)[0].attachment = swapchain->getDrawable();
        commandBuffer->beginRenderPass(renderPass.get());
        commandBuffer->bindPipeline(pipeline.get());
        commandBuffer->bindShaderParameter(1, shaders::ShaderStage::Vertex, buffers[currentFrame].uniformShaderParameter.get());
        commandBuffer->bindShaderParameter(2, shaders::ShaderStage::Fragment, textureShaderParameter.get());
        commandBuffer->bindVertexBuffer(0, vertexBuffer.get(), 0);
        commandBuffer->viewport(0.0, 0.0, drawData->DisplaySize.x * drawData->FramebufferScale.x,
                                drawData->DisplaySize.y * drawData->FramebufferScale.y, 0.0, 1.0);

        ImVec2 clipOffset = drawData->DisplayPos;      // (0,0) unless using multi-viewports
        ImVec2 clipScale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        auto framebufferWidth = static_cast<uint32_t>(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        auto framebufferHeight = static_cast<uint32_t>(drawData->DisplaySize.y * drawData->FramebufferScale.y);

        std::size_t vertexOffset = 0;
        std::size_t indexOffset = 0;
        for (auto& list : commandLists) {
            for (int i = 0; i < list->CmdBuffer.Size; ++i) {
                const auto& cmd = list->CmdBuffer[i];

                if (cmd.ElemCount == 0) { // drawIndexed doesn't accept this
                    continue;
                }

                glm::u32vec2 clipMin = { std::max(0U, static_cast<uint32_t>((cmd.ClipRect.x - clipOffset.x) * clipScale.x)),
                                         std::max(0U, static_cast<uint32_t>((cmd.ClipRect.y - clipOffset.y) * clipScale.y)) };
                glm::u32vec2 clipMax = { std::min(framebufferWidth, static_cast<uint32_t>((cmd.ClipRect.z - clipOffset.x) * clipScale.x)),
                                         std::min(framebufferHeight,
                                                  static_cast<uint32_t>((cmd.ClipRect.w - clipOffset.y) * clipScale.y)) };

                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y) {
                    continue;
                }

                commandBuffer->scissor(clipMin.x, clipMin.y, clipMax.x - clipMin.x, clipMax.y - clipMin.y);

                commandBuffer->setVertexBufferOffset(0, (vertexOffset + cmd.VtxOffset) * sizeof(ImDrawVert));
                commandBuffer->bindIndexBuffer(indexBuffer.get(),
                                               sizeof(ImDrawIdx) == 2 ? rapi::IndexType::UINT16 : rapi::IndexType::UINT32, // NOLINT
                                               (cmd.IdxOffset + static_cast<uint32_t>(indexOffset)) * sizeof(ImDrawIdx));
                commandBuffer->drawIndexed(cmd.ElemCount, 0);
            }

            indexOffset += list->IdxBuffer.Size;
            vertexOffset += list->VtxBuffer.Size;
        }

        commandBuffer->endRenderPass();
    }
}

void kc::ImGuiRenderer::newFrame() {
    ZoneScoped;
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void kc::ImGuiRenderer::endFrame() {
    ZoneScoped;
    ImGui::EndFrame();
}

void kc::ImGuiRenderer::updateUniformBuffer(rapi::IBuffer* uniformBuffer, const ImVec2& displaySize, const ImVec2& displayPos) {
    ZoneScoped;
    uniforms.scale.x = 2.0F / displaySize.x;
    uniforms.scale.y = 2.0F / displaySize.y;
    uniforms.translate.x = -1.0F - displayPos.x * uniforms.scale.x;
    uniforms.translate.y = -1.0F - displayPos.y * uniforms.scale.y;

    uniformBuffer->mapMemory([this](void* data) { std::memcpy(data, &uniforms, sizeof(ImGuiShaderUniforms)); });
}
