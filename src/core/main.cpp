#include <iostream>

#include <fmt/core.h>
#include <glm/glm.hpp>

#include <mesh/mesh.hpp>
#include <mesh/scene.hpp>
#include <rapi/rapi.hpp>
#include <rapi/window.hpp>

// This is just a simple triangles
std::vector<krypton::mesh::Vertex> vertexData = {
    {.pos = {  0.0,  1.0,  0.0,  1.0 }, .color = {0,0,1,1}},
    {.pos = { -1.0, -1.0,  0.0,  1.0 }, .color = {1,0,0,1}},
    {.pos = {  1.0, -1.0,  0.0,  1.0 }, .color = {0,1,0,1}}
};

krypton::mesh::Primitive primitive = {
    .vertices = vertexData,
    .indices = { 0, 1, 2 },
    .materialIndex = 0,
};

auto main(int argc, char* argv[]) -> int {
    try {
        auto smesh = std::make_shared<krypton::mesh::Mesh>();
        smesh->primitives.push_back(primitive);

        auto rapi = std::move(krypton::rapi::getRenderApi());
        rapi->init();

        auto meshHandle = rapi->createRenderObject();
        rapi->loadMeshForRenderObject(meshHandle, smesh);

        while (!rapi->getWindow()->shouldClose()) {
            rapi->getWindow()->pollEvents();
            rapi->beginFrame();
            rapi->render(meshHandle);
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
