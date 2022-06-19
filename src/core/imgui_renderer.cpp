#include <span>
#include <utility>

#include <Tracy.hpp>

#include <core/imgui_renderer.hpp>
#include <rapi/icommandbuffer.hpp>
#include <rapi/irenderpass.hpp>
#include <rapi/window.hpp>

namespace kc = krypton::core;

kc::ImGuiRenderer::ImGuiRenderer(std::shared_ptr<rapi::RenderAPI> rapi) : uniforms({}), rapi(std::move(rapi)) {}

void kc::ImGuiRenderer::buildFontTexture(ImGuiIO& io) {
    ZoneScoped;
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    fontAtlas = rapi->createTexture(rapi::TextureUsage::SampledImage);
    fontAtlas->setName(u8"ImGui font texture");
    fontAtlas->setColorEncoding(rapi::ColorEncoding::LINEAR);
    fontAtlas->uploadTexture(width, height,
                             std::span { reinterpret_cast<std::byte*>(pixels), static_cast<std::size_t>(width * height * 4) },
                             rapi::TextureFormat::RGBA8);

    if (fontAtlasSampler == nullptr) {
        // This likely won't change.
        fontAtlasSampler = rapi->createSampler();
        fontAtlasSampler->setName(u8"ImGui font-atlas sampler");
        fontAtlasSampler->createSampler();
    }

    io.Fonts->SetTexID(static_cast<ImTextureID>(fontAtlas.get()));
}

void kc::ImGuiRenderer::init() {
    ZoneScopedN("ImGuiRenderer::init");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    rapi->getWindow()->initImgui();

    auto& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendRendererName = "krypton::core::ImGuiRenderer";
    io.BackendPlatformName = "krypton::rapi::RenderAPI";

    buildFontTexture(io);

    // Create the index/vertex buffers
    vertexBuffer = rapi->createBuffer();
    indexBuffer = rapi->createBuffer();

    vertexBuffer->setName(u8"ImGui Vertex Buffer");
    indexBuffer->setName(u8"ImGui Index Buffer");

    vertexBuffer->create(sizeof(ImDrawVert), rapi::BufferUsage::UniformBuffer, rapi::BufferMemoryLocation::SHARED);
    indexBuffer->create(sizeof(ImDrawIdx), rapi::BufferUsage::UniformBuffer, rapi::BufferMemoryLocation::SHARED);

    // Create the shaders
    auto fragGlsl = krypton::shaders::readShaderFile("shaders/ui.frag");
    auto vertGlsl = krypton::shaders::readShaderFile("shaders/ui2.vert");

    fragmentShader =
        rapi->createShaderFunction({ reinterpret_cast<const std::byte*>(fragGlsl.content.data()), fragGlsl.content.size() + 1 },
                                   krypton::shaders::ShaderSourceType::GLSL, krypton::shaders::ShaderStage::Fragment);
    vertexShader = rapi->createShaderFunction({ reinterpret_cast<const std::byte*>(vertGlsl.content.data()), vertGlsl.content.size() + 1 },
                                              krypton::shaders::ShaderSourceType::GLSL, krypton::shaders::ShaderStage::Vertex);

    if (fragmentShader->needsTranspile())
        fragmentShader->transpile("main", krypton::shaders::ShaderStage::Fragment);
    if (vertexShader->needsTranspile())
        vertexShader->transpile("main", krypton::shaders::ShaderStage::Vertex);

    fragmentShader->createModule();
    vertexShader->createModule();

    // Create the render pass
    renderPass = rapi->createRenderPass();
    renderPass->setFragmentFunction(fragmentShader.get());
    renderPass->setVertexFunction(vertexShader.get());
    // clang-format off
    renderPass->setVertexDescriptor({
        .buffers = {
            {
                .stride = sizeof(ImDrawVert),
                .inputRate = rapi::VertexInputRate::Vertex,
            },
        },
        .attributes = {
            {
                .offset = offsetof(ImDrawVert, pos),
                .bufferIndex = 0,
                .format = rapi::VertexFormat::RG32_FLOAT,
            },
            {
                .offset = offsetof(ImDrawVert, uv),
                .bufferIndex = 0,
                .format = rapi::VertexFormat::RG32_FLOAT,
            },
            {
                .offset = offsetof(ImDrawVert, col),
                .bufferIndex = 0,
                .format = rapi::VertexFormat::RGBA8_UNORM,
            },
        }
    });
    renderPass->addAttachment(0, {
        .attachment = rapi->getRenderTargetTextureHandle(),
        .attachmentFormat = rapi::TextureFormat::BGRA10,
        .loadAction = krypton::rapi::AttachmentLoadAction::Load,
        .storeAction = krypton::rapi::AttachmentStoreAction::Store,
        .clearColor = glm::fvec4(0.0),
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
    renderPass->build();

    // Build our uniform buffer
    uniformBuffer = rapi->createBuffer();
    uniformBuffer->setName(u8"ImGui Uniform Buffer");
    uniformBuffer->create(sizeof(ImGuiShaderUniforms), rapi::BufferUsage::UniformBuffer, rapi::BufferMemoryLocation::SHARED);

    updateUniformBuffer(io.DisplaySize, ImVec2(0, 0));

    uniformShaderParameter = rapi->createShaderParameter();
    uniformShaderParameter->addBuffer(0, uniformBuffer);
    uniformShaderParameter->buildParameter();

    textureShaderParameter = rapi->createShaderParameter();
    textureShaderParameter->addTexture(0, fontAtlas);
    textureShaderParameter->addSampler(1, fontAtlasSampler);
    textureShaderParameter->buildParameter();
}

void kc::ImGuiRenderer::destroy() {
    uniformBuffer->destroy();
}

void kc::ImGuiRenderer::draw(rapi::ICommandBuffer* commandBuffer) {
    ZoneScoped;
    ImGui::Render();
    auto* drawData = ImGui::GetDrawData();

    // Copy all vertex and index buffers into the proper buffers. Because of Vulkan, we cannot copy
    // buffers while within a render pass.
    if (drawData->TotalVtxCount > 0) {
        // Update the uniform buffer
        updateUniformBuffer(drawData->DisplaySize, drawData->DisplayPos);

        size_t vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
        size_t indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

        // We will have to resize the buffers if they're not large enough for all the data.
        if (vertexBufferSize > vertexBuffer->getSize()) {
            vertexBuffer->destroy();
            vertexBuffer->create(vertexBufferSize, rapi::BufferUsage::UniformBuffer, rapi::BufferMemoryLocation::SHARED);
        }

        if (indexBufferSize > indexBuffer->getSize()) {
            indexBuffer->destroy();
            indexBuffer->create(vertexBufferSize, rapi::BufferUsage::UniformBuffer, rapi::BufferMemoryLocation::SHARED);
        }

        // Copy the vertex and index buffers
        vertexBuffer->mapMemory([this, drawData](void* vtxData) {
            indexBuffer->mapMemory([drawData, vtxData](void* idxData) {
                auto* vertexDestination = static_cast<ImDrawVert*>(vtxData);
                auto* indexDestination = static_cast<ImDrawIdx*>(idxData);
                for (int i = 0; i < drawData->CmdListsCount; ++i) {
                    auto& list = drawData->CmdLists[i];

                    std::memcpy(vertexDestination, list->VtxBuffer.Data, list->VtxBuffer.Size * sizeof(ImDrawVert));
                    std::memcpy(indexDestination, list->IdxBuffer.Data, list->IdxBuffer.Size * sizeof(ImDrawIdx));

                    vertexDestination += list->VtxBuffer.Size;
                    indexDestination += list->IdxBuffer.Size;
                }
            });
        });

        commandBuffer->beginRenderPass(renderPass.get());
        // The vertex buffer occupies index 0 in Metal.
        commandBuffer->bindShaderParameter(1, shaders::ShaderStage::Vertex, uniformShaderParameter.get());
        commandBuffer->bindShaderParameter(2, shaders::ShaderStage::Fragment, textureShaderParameter.get());
        commandBuffer->viewport(0.0, 0.0, drawData->DisplaySize.x * drawData->FramebufferScale.x,
                                drawData->DisplaySize.y * drawData->FramebufferScale.y, 0.0, 1.0);

        ImVec2 clipOffset = drawData->DisplayPos;      // (0,0) unless using multi-viewports
        ImVec2 clipScale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        auto framebufferWidth = static_cast<uint32_t>(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        auto framebufferHeight = static_cast<uint32_t>(drawData->DisplaySize.y * drawData->FramebufferScale.y);

        std::size_t vertexOffset = 0, indexOffset = 0;
        for (int n = 0; n < drawData->CmdListsCount; ++n) {
            const ImDrawList* cmdList = drawData->CmdLists[n];

            for (int i = 0; i < cmdList->CmdBuffer.Size; ++i) {
                const auto& cmd = cmdList->CmdBuffer[i];

                if (cmd.ElemCount == 0) // drawIndexed doesn't accept this
                    continue;

                glm::u32vec2 clipMin = { (cmd.ClipRect.x - clipOffset.x) * clipScale.x, (cmd.ClipRect.y - clipOffset.y) * clipScale.y };
                glm::u32vec2 clipMax = { (cmd.ClipRect.z - clipOffset.x) * clipScale.x, (cmd.ClipRect.w - clipOffset.y) * clipScale.y };

                if (clipMin.x < 0) {
                    clipMin.x = 0;
                }
                if (clipMin.y < 0) {
                    clipMin.y = 0;
                }
                if (clipMax.x > framebufferWidth) {
                    clipMax.x = framebufferWidth;
                }
                if (clipMax.y > framebufferHeight) {
                    clipMax.y = framebufferHeight;
                }
                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                    continue;

                commandBuffer->scissor(clipMin.x, clipMin.y, clipMax.x - clipMin.x, clipMax.y - clipMin.y);

                commandBuffer->bindVertexBuffer(vertexBuffer.get(), (vertexOffset + cmd.VtxOffset) * sizeof(ImDrawVert));
                commandBuffer->drawIndexed(indexBuffer.get(), cmd.ElemCount,
                                           sizeof(ImDrawIdx) == 2 ? rapi::IndexType::UINT16 : rapi::IndexType::UINT32, // NOLINT
                                           (cmd.IdxOffset + indexOffset) * sizeof(ImDrawIdx));
            }

            indexOffset += cmdList->IdxBuffer.Size;
            vertexOffset += cmdList->VtxBuffer.Size;
        }

        commandBuffer->endRenderPass();
    }
}

void kc::ImGuiRenderer::newFrame() {
    ZoneScoped;
    ImGui::NewFrame();
}

void kc::ImGuiRenderer::endFrame() {
    ZoneScoped;
    ImGui::EndFrame();
}

void kc::ImGuiRenderer::updateUniformBuffer(const ImVec2& displaySize, const ImVec2& displayPos) {
    ZoneScoped;
    uniforms.scale.x = 2.0f / displaySize.x;
    uniforms.scale.y = 2.0f / displaySize.y;
    uniforms.translate.x = -1.0f - displayPos.x * uniforms.scale.x;
    uniforms.translate.y = -1.0f - displayPos.y * uniforms.scale.y;

    uniformBuffer->mapMemory([this](void* data) { std::memcpy(data, &uniforms, sizeof(ImGuiShaderUniforms)); });
}
