#include <filesystem>
#include <iostream>
#include <mutex>
#include <vector>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>

#include <mesh/mesh.hpp>
#include <mesh/scene.hpp>
#include <models/fileloader.hpp>
#include <rapi/rapi.hpp>
#include <rapi/window.hpp>
#include <threading/scheduler.hpp>
#include <util/handle.hpp>
#include <util/logging.hpp>

// Fuck windows.h
#undef near
#undef far

namespace fs = std::filesystem;

std::shared_ptr<krypton::rapi::CameraData> cameraData = nullptr;

std::vector<krypton::util::Handle<"RenderObject">> renderObjectHandles = {};
std::mutex renderObjectHandleMutex;

std::string modelPathString;

float cameraPos[3] = { 10, 0, -1 };
float focus[3] = { 0, 0, 0 };

void loadModel(krypton::rapi::RenderAPI* rapi, const fs::path& path) {
    kt::Scheduler::getInstance().run([rapi, path]() {
        auto fileLoader = std::make_unique<krypton::models::FileLoader>();
        auto loaded = fileLoader->loadFile(path);

        if (!loaded) {
            krypton::log::err("Failed to load file!");
        } else {
            std::vector<krypton::util::Handle<"Material">> localMaterialHandles;
            for (auto& mat : fileLoader->materials) {
                localMaterialHandles.push_back(rapi->createMaterial(mat));
            }

            for (auto& mesh : fileLoader->meshes) {
                // We lock here because the buildRenderObject can take a bit and
                // we'd otherwise be blocking our main thread.
                renderObjectHandleMutex.lock();
                auto& handle = renderObjectHandles.emplace_back(rapi->createRenderObject());

                for (auto& primitive : mesh->primitives) {
                    rapi->addPrimitive(handle, primitive, localMaterialHandles[primitive.materialIndex]);
                }

                rapi->setObjectTransform(handle, mesh->transform);
                rapi->setObjectName(handle, mesh->name);
                rapi->buildRenderObject(handle);
                renderObjectHandleMutex.unlock();
            }
        }
    });
}

void drawUi(krypton::rapi::RenderAPI* rapi) {
    return;
    ImGui::Begin("Main");

    ImGui::SliderFloat3("Camera Position", cameraPos, -25.0, 25.0);
    ImGui::SliderFloat3("Camera Focus", focus, -25.0, 25.0);
    ImGui::SliderFloat("Camera Far", &cameraData->far, 1.0f, 1000.0f);
    cameraData->view = glm::lookAt(glm::make_vec3(cameraPos), // position in world
                                   glm::make_vec3(focus),     // look at position; center of world
                                   glm::vec3(0, 1, 0));       // up vector - Y

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

        auto rapi = krypton::rapi::getRenderApi();
        rapi->init();
        cameraData = rapi->getCameraData();

        int width, height;
        rapi->getWindow()->getWindowSize(&width, &height);

        cameraData->projection = glm::perspective(glm::radians(70.0f), (float)width / (float)height, cameraData->near, cameraData->far);
        cameraData->view = glm::lookAt(glm::vec3(10, 0, -1), // position in world
                                       glm::vec3(0, 0, 0),   // look at position; center of world
                                       glm::vec3(0, 1, 0));  // up vector - Y

        while (!rapi->getWindow()->shouldClose()) {
            rapi->beginFrame();

            renderObjectHandleMutex.lock();
            for (auto& handle : renderObjectHandles) {
                rapi->render(handle);
            }
            renderObjectHandleMutex.unlock();

            drawUi(rapi.get());

            rapi->drawFrame();
            rapi->endFrame();
        }

        for (auto& handle : renderObjectHandles) {
            if (!rapi->destroyRenderObject(handle))
                krypton::log::err("Failed to destroy a render object handle");
        }

        rapi->shutdown();

        kt::Scheduler::getInstance().shutdown();
    } catch (const std::exception& e) {
        krypton::log::err("Exception occured: {}", e.what());
    }
    return 0;
}
