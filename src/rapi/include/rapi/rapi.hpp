#pragma once

#include <memory>

#include <mesh/mesh.hpp>

#include <rapi/camera.hpp>
#include <rapi/render_object_handle.hpp>
#include <rapi/window.hpp>

struct ImDrawData;

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

        virtual void beginFrame() = 0;

        /**
         * Creates a new handle to a new render object. This handle can from now
         * on be used to modify
         */
        [[nodiscard]] virtual auto createRenderObject() -> RenderObjectHandle = 0;

        [[nodiscard]] virtual auto destroyRenderObject(RenderObjectHandle& handle) -> bool = 0;

        /**
         * The UI has to be constructed through ImGui before calling this.
         * This auto calls ImGui::Render() and renders the relevant draw data.
         */
        virtual void drawFrame() = 0;

        virtual void endFrame() = 0;

        [[nodiscard]] virtual auto getWindow() -> std::shared_ptr<krypton::rapi::Window> = 0;

        /**
         * Creates the window and initializes the rendering
         * backend. After this, everything is good to go and
         * this API can be used for rendering.
         */
        virtual void init() = 0;

        /**
         * Loads given mesh into the render object for given handle.
         */
        virtual void loadMeshForRenderObject(RenderObjectHandle& handle, std::shared_ptr<krypton::mesh::Mesh> mesh) = 0;

        /**
         * Adds given mesh to the render queue for this frame.
         */
        virtual void render(RenderObjectHandle& handle) = 0;

        virtual void resize(int width, int height) = 0;

        virtual void setCameraData(std::shared_ptr<krypton::rapi::CameraData> cameraData) = 0;

        /**
         * Shutdowns the rendering backend and makes it useless
         * for further rendering commands.
         */
        virtual void shutdown() = 0;
    };
} // namespace krypton::rapi
