#pragma once

#include <memory>

#include <mesh.hpp>

#include "window.hpp"

namespace krypton::rapi {
    class RenderAPI;

    std::unique_ptr<krypton::rapi::RenderAPI> getRenderApi();

    /**
     * The RenderAPI interface that can be extended to provide different
     * render backends.
     */
    class RenderAPI {
    public:
        virtual ~RenderAPI() {}

        virtual void drawFrame() = 0;

        virtual auto getWindow() -> std::shared_ptr<krypton::rapi::Window> = 0;

        /**
         * Creates the window and initializes the rendering
         * backend. After this, everything is good to go and
         * this API can be used for rendering.
         */
        virtual void init() = 0;

        /**
         * Adds given mesh to the render queue. This only has to be called
         * once per mesh, as the mesh will be rendered for every frame. 
         */
        virtual void render(std::shared_ptr<krypton::mesh::Mesh> mesh) = 0;

        virtual void resize(int width, int height) = 0;

        /**
         * Shutdowns the rendering backend and makes it useless
         * for further rendering commands.
         */
        virtual void shutdown() = 0;
    };
}
