#pragma once

#include <memory>

#include <mesh/mesh.hpp>

#include <rapi/camera.hpp>
#include <rapi/window.hpp>

#include <util/handle.hpp>

namespace kr = krypton::rapi;

namespace krypton::rapi {
    class RenderAPI;

    std::unique_ptr<krypton::rapi::RenderAPI> getRenderApi();

    /**
     * The RenderAPI interface that can be extended to provide different
     * render backends.
     */
    class RenderAPI {
    public:
        virtual ~RenderAPI() = default;

        virtual void addPrimitive(util::Handle<"RenderObject">& handle, krypton::mesh::Primitive& primitive,
                                  util::Handle<"Material">& material) = 0;

        virtual void beginFrame() = 0;

        /**
         * This finalizes the process of creating a render object. If a object has been built
         * previously, it has to be rebuilt after adding new primitives or changing the transform.
         */
        virtual void buildRenderObject(util::Handle<"RenderObject">& handle) = 0;

        /**
         * Creates a new handle to a new render object. This handle can from now
         * on be used to modify and update a single object.
         */
        [[nodiscard]] virtual auto createRenderObject() -> util::Handle<"RenderObject"> = 0;

        [[nodiscard]] virtual auto createMaterial(krypton::mesh::Material material) -> util::Handle<"Material"> = 0;

        [[nodiscard]] virtual bool destroyRenderObject(util::Handle<"RenderObject">& handle) = 0;

        [[nodiscard]] virtual bool destroyMaterial(util::Handle<"Material">& handle) = 0;

        /**
         * The UI has to be constructed through ImGui before calling this.
         * This auto calls ImGui::Render() and renders the relevant draw data.
         */
        virtual void drawFrame() = 0;

        virtual void endFrame() = 0;

        [[nodiscard]] virtual auto getCameraData() -> std::shared_ptr<krypton::rapi::CameraData> = 0;

        [[nodiscard]] virtual auto getWindow() -> std::shared_ptr<krypton::rapi::Window> = 0;

        /**
         * Creates the window and initializes the rendering
         * backend. After this, everything is good to go and
         * this API can be used for rendering.
         */
        virtual void init() = 0;

        /**
         * Adds given mesh to the render queue for this frame.
         */
        virtual void render(util::Handle<"RenderObject"> handle) = 0;

        virtual void resize(int width, int height) = 0;

        /**
         * Set the name of a RenderObject. This mainly helps for debugging
         * purposes but can also be used in the GUI.
         */
        virtual void setObjectName(util::Handle<"RenderObject">& handle, std::string name) = 0;

        virtual void setObjectTransform(util::Handle<"RenderObject">& handle, glm::mat4x3 transform) = 0;

        /**
         * Shutdowns the rendering backend and makes it useless
         * for further rendering commands.
         */
        virtual void shutdown() = 0;
    };
} // namespace krypton::rapi
