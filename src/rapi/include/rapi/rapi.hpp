#pragma once

#include <memory>
#include <span>

#include <assets/mesh.hpp>

#include <rapi/camera.hpp>
#include <rapi/color_encoding.hpp>
#include <rapi/textures.hpp>
#include <rapi/window.hpp>

#include <util/consteval_pow.hpp>
#include <util/handle.hpp>

namespace krypton::rapi {
    class RenderAPI;

    [[nodiscard]] std::shared_ptr<RenderAPI> getRenderApi(Backend backend) noexcept(false);

    // The default backend for this platform/build. The list of backends supported on this
    // platform/build might only include this, but might include more backends.
    [[nodiscard]] consteval Backend getPlatformDefaultBackend() noexcept {
#ifdef __APPLE__
        return Backend::Metal;
#elif defined(__linux__) || defined(WIN32)
        return Backend::Vulkan;
#else
        return Backend::None;
#endif
    }

    // A list of backends that are supported on this platform/build
    [[nodiscard]] constexpr Backend getPlatformSupportedBackends() noexcept;

    /**
     * The RenderAPI interface that can be extended to provide different
     * render backends.
     */
    class RenderAPI : public std::enable_shared_from_this<RenderAPI> {
    protected:
        constexpr static const auto maxNumOfMaterials = util::consteval_pow(2ULL, 15ULL);

        RenderAPI() = default;

        virtual std::shared_ptr<RenderAPI> getPointer() noexcept;

    public:
        virtual ~RenderAPI() = default;

        virtual void addPrimitive(const util::Handle<"RenderObject">& handle, assets::Primitive& primitive,
                                  const util::Handle<"Material">& material) = 0;

        virtual void beginFrame() = 0;

        virtual void buildMaterial(const util::Handle<"Material">& handle) = 0;

        /**
         * This finalizes the process of creating a render object. If a object has been built
         * previously, it has to be rebuilt after adding new primitives or changing the transform.
         */
        virtual void buildRenderObject(const util::Handle<"RenderObject">& handle) = 0;

        /**
         * Creates a new handle to a new render object. This handle can from now
         * on be used to modify and update a single object.
         */
        [[nodiscard]] virtual auto createRenderObject() -> util::Handle<"RenderObject"> = 0;

        [[nodiscard]] virtual auto createMaterial() -> util::Handle<"Material"> = 0;

        [[nodiscard]] virtual auto createTexture() -> util::Handle<"Texture"> = 0;

        [[nodiscard]] virtual bool destroyRenderObject(util::Handle<"RenderObject">& handle) = 0;

        [[nodiscard]] virtual bool destroyMaterial(util::Handle<"Material">& handle) = 0;

        [[nodiscard]] virtual bool destroyTexture(util::Handle<"Texture">& handle) = 0;

        /**
         * The UI has to be constructed through ImGui before calling this.
         * This auto calls ImGui::Render() and renders the relevant draw data.
         */
        virtual void drawFrame() = 0;

        virtual void endFrame() = 0;

        [[nodiscard]] virtual auto getCameraData() -> std::shared_ptr<CameraData> = 0;

        [[nodiscard]] virtual auto getWindow() -> std::shared_ptr<Window> = 0;

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

        virtual void setMaterialBaseColor(const util::Handle<"Material">& handle, glm::fvec4 baseColor) = 0;

        virtual void setMaterialDiffuseTexture(const util::Handle<"Material">& handle, util::Handle<"Texture"> textureHandle) = 0;

        /**
         * Set the name of a RenderObject. This mainly helps for debugging
         * purposes but can also be used in the GUI.
         */
        virtual void setObjectName(const util::Handle<"RenderObject">& handle, std::string name) = 0;

        virtual void setObjectTransform(const util::Handle<"RenderObject">& handle, glm::mat4x3 transform) = 0;

        virtual void setTextureColorEncoding(const util::Handle<"Texture">& handle, ColorEncoding colorEncoding) = 0;

        virtual void setTextureData(const util::Handle<"Texture">& handle, uint32_t width, uint32_t height, std::span<std::byte> pixels,
                                    TextureFormat format) = 0;

        virtual void setTextureName(const util::Handle<"Texture">& handle, std::string name) = 0;

        virtual void setTextureUsage(const util::Handle<"Texture">& handle, TextureUsage usage) = 0;

        /**
         * Shutdowns the rendering backend and makes it useless for further rendering commands.
         */
        virtual void shutdown() = 0;

        /**
         * This finalizes the texture creation process by uploading the texture to the GPU. To be
         * consistent, this function returns *after* the texture has been fully uploaded, so it's
         * best to call this from a separate thread.
         */
        virtual void uploadTexture(const util::Handle<"Texture">& handle) = 0;
    };

    static_assert(std::is_abstract<RenderAPI>());
} // namespace krypton::rapi
