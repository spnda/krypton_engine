#pragma once

#include <memory>

#include "window.hpp"

namespace krypton::rapi {
    class RenderAPI;

    std::shared_ptr<krypton::rapi::RenderAPI> getRenderApi();

    /**
     * The RenderAPI interface that can be extended to provide different
     * render backends.
     */
    class RenderAPI {
    protected:
        krypton::rapi::Window window;

    public:
        RenderAPI();

        /**
         * Creates the window and initializes the rendering
         * backend. After this, everything is good to go and
         * this API can be used for rendering.
         */
        virtual void init() = 0;

        /**
         * Shutdowns the rendering backend and makes it useless
         * for further rendering commands.
         */
        virtual void shutdown() = 0;
    };
}
