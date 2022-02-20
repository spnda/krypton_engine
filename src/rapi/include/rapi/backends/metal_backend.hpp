#pragma once

#ifdef RAPI_WITH_METAL

#include <memory>

#include <mesh/mesh.hpp>

#include <rapi/object_handles.hpp>
#include <rapi/rapi.hpp>
#include <rapi/window.hpp>

namespace krypton::rapi {
    /**
     * supposed to be a C++ header and most Metal related API is Objective-C.
     */
    class Metal_RAPI final : public RenderAPI {
        std::shared_ptr<krypton::rapi::Window> window;

        std::vector<krypton::rapi::RenderObjectHandle> handlesForFrame = {};

        std::shared_ptr<krypton::rapi::CameraData> cameraData;

    public:
        Metal_RAPI();
        ~Metal_RAPI();

        void beginFrame() override;
        auto createRenderObject() -> RenderObjectHandle override;
        auto createMaterial(krypton::mesh::Material material) -> MaterialHandle override;
        auto destroyRenderObject(RenderObjectHandle& handle) -> bool override;
        bool destroyMaterial(MaterialHandle& handle) override;
        void drawFrame() override;
        void endFrame() override;
        auto getCameraData() -> std::shared_ptr<krypton::rapi::CameraData> override;
        auto getWindow() -> std::shared_ptr<krypton::rapi::Window> override;
        void init() override;
        void loadMeshForRenderObject(RenderObjectHandle& handle, std::shared_ptr<krypton::mesh::Mesh> mesh) override;
        void render(RenderObjectHandle handle) override;
        void resize(int width, int height) override;
        void shutdown() override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_METAL
