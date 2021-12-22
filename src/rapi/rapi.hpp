#pragma once

#include <memory>

#include "window.hpp"

namespace krypton::rapi {
    class RenderAPI;

    std::unique_ptr<krypton::rapi::RenderAPI> getRenderApi();

    /**
     * The RenderAPI interface that can be extended to provide different
     * render backends.
     */
    class RenderAPI {
    protected:

    public:
        krypton::rapi::Window window;

        RenderAPI();

        virtual void drawFrame() = 0;

        /**
         * Creates the window and initializes the rendering
         * backend. After this, everything is good to go and
         * this API can be used for rendering.
         */
        virtual void init() = 0;

        virtual void resize(int width, int height) = 0;

        /**
         * Shutdowns the rendering backend and makes it useless
         * for further rendering commands.
         */
        virtual void shutdown() = 0;
    };
}
