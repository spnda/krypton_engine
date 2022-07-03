#include <filesystem>
#include <fstream>
#include <vector>

#include <Tracy.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>

#include <assets/loader/fileloader.hpp>
#include <assets/mesh.hpp>
#include <core/imgui_renderer.hpp>
#include <rapi/icommandbuffer.hpp>
#include <rapi/iqueue.hpp>
#include <rapi/irenderpass.hpp>
#include <rapi/isemaphore.hpp>
#include <rapi/iswapchain.hpp>
#include <rapi/itexture.hpp>
#include <rapi/rapi.hpp>
#include <rapi/window.hpp>
#include <shaders/shaders.hpp>
#include <util/logging.hpp>
#include <util/scheduler.hpp>

// Fuck windows.h
#undef near
#undef far

namespace fs = std::filesystem;
namespace kt = krypton::threading;
namespace kr = krypton::rapi;

std::string modelPathString;

struct FrameData {
    std::shared_ptr<kr::ISemaphore> imageAcquireSemaphore;
    std::shared_ptr<kr::ISemaphore> renderSemaphore;
};

void loadModel(krypton::rapi::RenderAPI* rapi, const fs::path& path) {
    ZoneScoped;
    kt::Scheduler::getInstance().run([rapi, path]() {
        ZoneScopedN("Threaded loadModel");
        auto fileLoader = std::make_unique<krypton::assets::loader::FileLoader>();
        auto loaded = fileLoader->loadFile(path);

        if (!loaded) {
            krypton::log::err("Failed to load file!");
        }

        // TODO: Implement new model loading
    });
}

void drawUi(krypton::rapi::RenderAPI* rapi) {
    ZoneScoped;
    ImGui::ShowDemoWindow();

    ImGui::Begin("Main");

    ImGui::Separator();

    ImGui::InputText("Model path", &modelPathString);
    if (ImGui::Button("Add model") && !modelPathString.empty()) {
        loadModel(rapi, fs::path { modelPathString });
        modelPathString.clear();
    }

    ImGui::End();
}

auto main(int argc, char* argv[]) -> int {
    try {
        kt::Scheduler::getInstance().start();

        // We currently just use the default backend for the current platform.
        // auto rapi = krypton::rapi::getRenderApi(krypton::rapi::getPlatformDefaultBackend());
        auto rapi = krypton::rapi::getRenderApi(krypton::rapi::Backend::Vulkan);
        rapi->init();

        auto device = rapi->getSuitableDevice({});
        krypton::log::log("Launching on {}", device->getDeviceName());

        auto window = rapi->getWindow();
        window->pollEvents(); // Let the window open instantly.

        auto swapchain = device->createSwapchain(window.get());
        swapchain->create(krypton::rapi::TextureUsage::ColorRenderTarget);

        bool needsResize = false;
        std::vector<FrameData> frameData(swapchain->getImageCount());
        for (auto& i : frameData) {
            i.imageAcquireSemaphore = device->createSemaphore();
            i.imageAcquireSemaphore->create();
            i.renderSemaphore = device->createSemaphore();
            i.renderSemaphore->create();
        }

        auto imgui = std::make_unique<krypton::core::ImGuiRenderer>(device, window.get());
        imgui->init();

        auto fragSpirv = krypton::shaders::readBinaryShaderFile("shaders/frag.spv");
        auto vertSpirv = krypton::shaders::readBinaryShaderFile("shaders/vert.spv");

        auto defaultFragmentFunction =
            device->createShaderFunction({ reinterpret_cast<const std::byte*>(fragSpirv.data()), fragSpirv.size() },
                                         krypton::shaders::ShaderSourceType::SPIRV, krypton::shaders::ShaderStage::Fragment);
        auto defaultVertexFunction =
            device->createShaderFunction({ reinterpret_cast<const std::byte*>(vertSpirv.data()), vertSpirv.size() },
                                         krypton::shaders::ShaderSourceType::SPIRV, krypton::shaders::ShaderStage::Vertex);

        if (defaultFragmentFunction->needsTranspile())
            defaultFragmentFunction->transpile("main0", krypton::shaders::ShaderStage::Fragment);
        if (defaultVertexFunction->needsTranspile())
            defaultVertexFunction->transpile("main0", krypton::shaders::ShaderStage::Vertex);

        defaultFragmentFunction->createModule();
        defaultVertexFunction->createModule();

        auto defaultRenderPass = device->createRenderPass();
        defaultRenderPass->setFragmentFunction(defaultFragmentFunction.get());
        defaultRenderPass->setVertexFunction(defaultVertexFunction.get());
        // clang-format off
        defaultRenderPass->setVertexDescriptor({
            .buffers = {
               {
                   .stride = sizeof(krypton::assets::Vertex),
                   .inputRate = krypton::rapi::VertexInputRate::Vertex,
               },
           },
            .attributes = {
                {
                    .offset = 0,
                    .bufferIndex = 0,
                    .format = krypton::rapi::VertexFormat::RGBA32_FLOAT,
                }
            }
        });
        defaultRenderPass->addAttachment(0, {
            // .attachment = rapi->getRenderTargetTextureHandle(),
            .attachmentFormat = krypton::rapi::TextureFormat::BGRA10,
            .loadAction = krypton::rapi::AttachmentLoadAction::Clear,
            .storeAction = krypton::rapi::AttachmentStoreAction::Store,
            .clearColor = glm::fvec4(0.0),
        });
        // clang-format on
        defaultRenderPass->build();

        auto presentationQueue = device->getPresentationQueue();
        presentationQueue->setName("PresentationQueue");

        auto commandPool = presentationQueue->createCommandPool();

        uint32_t currentFrame = 0;
        while (!window->shouldClose()) {
            ZoneScopedN("frameloop");
            FrameMark;
            if (window->isMinimised()) {
                window->waitEvents();
                continue;
            }

            if (window->isOccluded()) {
                // This is currently specific to macOS, where the OS will not want us to render
                // if the window is fully occluded. We can't use window->waitEvents here because
                // we're not waiting for a GLFW event. Therefore, we'll artificially slow down
                // the render loop to 1FPS.
                ZoneScopedN("isOccluded -> this_thread::sleep_for");
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1000ms);
            }

            auto commandBuffers = commandPool->allocateCommandBuffers(1);
            auto& cmd = commandBuffers[0];

            ++currentFrame %= swapchain->getImageCount();

            uint32_t imageIndex;
            std::unique_ptr<krypton::rapi::ITexture> drawable;
            if (!needsResize) {
                window->pollEvents();

                drawable = swapchain->nextImage(frameData[currentFrame].imageAcquireSemaphore.get(), &imageIndex, &needsResize);
            } else {
                window->waitEvents();
            }

            if (needsResize)
                continue;

            window->newFrame();
            imgui->newFrame();

            cmd->begin();
            cmd->beginRenderPass(defaultRenderPass.get());

            cmd->endRenderPass();

            drawUi(rapi.get());
            imgui->draw(cmd.get());
            cmd->end();

            presentationQueue->submit(cmd.get(), frameData[currentFrame].imageAcquireSemaphore.get(),
                                      frameData[currentFrame].renderSemaphore.get());
            swapchain->present(presentationQueue.get(), frameData[currentFrame].renderSemaphore.get(), &imageIndex, &needsResize);

            imgui->endFrame();
        }

        defaultRenderPass->destroy();

        imgui->destroy();
        rapi->shutdown();

        kl::log("rapi reference count: {}", rapi.use_count());

        kt::Scheduler::getInstance().shutdown();
    } catch (const std::exception& e) {
        kl::err("Exception occured: {}", e.what());
    }
    return 0;
}
