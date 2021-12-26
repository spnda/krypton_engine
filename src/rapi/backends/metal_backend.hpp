#pragma once

#ifdef RAPI_WITH_METAL

#include <memory>

#include <mesh.hpp>

#include "../rapi.hpp"

namespace krypton::rapi {
    /**
     * supposed to be a C++ header and most Metal related API is Objective-C.
     */
    class Metal_RAPI final : public RenderAPI {
        std::shared_ptr<krypton::rapi::Window> window;

    public:
        Metal_RAPI();
        ~Metal_RAPI();

        void drawFrame();
        auto getWindow() -> std::shared_ptr<krypton::rapi::Window>;
        void init();
        void render(std::shared_ptr<krypton::mesh::Mesh> mesh);
        void resize(int width, int height);
        void shutdown();
    };
}

#endif // #ifdef RAPI_WITH_METAL
