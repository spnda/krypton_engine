#include <filesystem>
#include <iostream>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>

#include <mesh/mesh.hpp>
#include <mesh/scene.hpp>
#include <models/fileloader.hpp>
#include <rapi/rapi.hpp>
#include <rapi/window.hpp>
#include <util/logging.hpp>

// Fuck windows.h
#undef near
#undef far

std::shared_ptr<krypton::rapi::CameraData> cameraData = nullptr;
float pos[3] = { 10, 0, -1 };
float focus[3] = { 0, 0, 0 };

void drawUi() {
    ImGui::Begin("Main");

    ImGui::Text("Hi from KHR_dynamic_rendering!");

    ImGui::SliderFloat3("Camera Position", pos, -25.0, 25.0);
    ImGui::SliderFloat3("Camera Focus", focus, -25.0, 25.0);
    cameraData->view = glm::lookAt(glm::make_vec3(pos),   // position in world
                                   glm::make_vec3(focus), // look at position; center of world
                                   glm::vec3(0, 1, 0));   // up vector - Y

    ImGui::End();

    ImGui::ShowDemoWindow();
}

auto main(int argc, char* argv[]) -> int {
    try {
        cameraData = std::make_shared<krypton::rapi::CameraData>();

        auto rapi = krypton::rapi::getRenderApi();
        rapi->setCameraData(cameraData);
        rapi->init();

        int width, height;
        rapi->getWindow()->getWindowSize(&width, &height);

        cameraData->projection = glm::perspective(glm::radians(70.0f), (float)width / (float)height, cameraData->near, cameraData->far);
        cameraData->view = glm::lookAt(glm::vec3(10, 0, -1), // position in world
                                       glm::vec3(0, 0, 0),   // look at position; center of world
                                       glm::vec3(0, 1, 0));  // up vector - Y

        std::vector<krypton::rapi::RenderObjectHandle> handles;
        auto fileLoader = std::make_unique<krypton::models::FileLoader>();
        auto loaded = fileLoader->loadFile("models/scene.gltf");
        if (!loaded) {
            krypton::log::err("Failed to load file!");
        } else {
            for (auto& mesh : fileLoader->meshes) {
                handles.push_back(rapi->createRenderObject());
                rapi->loadMeshForRenderObject(handles.back(), mesh);
            }
        }

        while (!rapi->getWindow()->shouldClose()) {
            rapi->getWindow()->pollEvents();
            rapi->beginFrame();

            for (auto& handle : handles) {
                rapi->render(handle);
            }

            drawUi();

            rapi->drawFrame();
            rapi->endFrame();
        }

        for (auto& handle : handles) {
            auto _ = rapi->destroyRenderObject(handle);
        }

        rapi->shutdown();
    } catch (const std::exception& e) {
        krypton::log::err("Exception occured: {}", e.what());
    }
    return 0;
}
