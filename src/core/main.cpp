#include <iostream>

#include <fmt/core.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>

#include <mesh/mesh.hpp>
#include <mesh/scene.hpp>
#include <rapi/rapi.hpp>
#include <rapi/window.hpp>

// This is just a simple triangles
std::vector<krypton::mesh::Vertex> vertexData = { { .pos = { 0.0, 1.0, 0.0, 1.0 }, .color = { 0, 0, 1, 1 } },
                                                  { .pos = { -1.0, -1.0, 0.0, 1.0 }, .color = { 1, 0, 0, 1 } },
                                                  { .pos = { 1.0, -1.0, 0.0, 1.0 }, .color = { 0, 1, 0, 1 } } };

krypton::mesh::Primitive primitive = {
    .vertices = vertexData,
    .indices = { 0, 1, 2 },
    .materialIndex = 0,
};

void drawUi() {
    ImGui::Begin("Main");

    ImGui::Text("Hi from KHR_dynamic_rendering!");

    ImGui::End();

    ImGui::ShowDemoWindow();
}

auto main(int argc, char* argv[]) -> int {
    try {
        auto smesh = std::make_shared<krypton::mesh::Mesh>();
        smesh->primitives.push_back(primitive);

        auto cameraData = std::make_shared<krypton::rapi::CameraData>();

        auto rapi = krypton::rapi::getRenderApi();
        rapi->setCameraData(cameraData);
        rapi->init();

        int width, height;
        rapi->getWindow()->getWindowSize(&width, &height);

        cameraData->projection = glm::perspective(glm::radians(70.0f), (float)width / (float)height, cameraData->near, cameraData->far);
        cameraData->view = glm::lookAt(glm::vec3(10, 0, 0), // position in world
                                       glm::vec3(0, 0, 0),  // look at position; center of world
                                       glm::vec3(0, 1, 0));

        auto meshHandle = rapi->createRenderObject();
        rapi->loadMeshForRenderObject(meshHandle, smesh);

        while (!rapi->getWindow()->shouldClose()) {
            rapi->getWindow()->pollEvents();
            rapi->beginFrame();

            rapi->render(meshHandle);
            drawUi();

            rapi->drawFrame();
            rapi->endFrame();
        }

        auto destroyed = rapi->destroyRenderObject(meshHandle);

        rapi->shutdown();
    } catch (const std::exception& e) {
        fmt::print(stderr, "Exception occured: {}\n", e.what());
    }
    return 0;
}
