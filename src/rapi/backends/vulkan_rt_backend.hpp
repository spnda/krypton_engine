#pragma once

#ifdef RAPI_WITH_VULKAN

#include <string>

#include <context.hpp>
#include <base/swapchain.hpp>
#include <resource/buffer.hpp>

#include "../rapi.hpp"
#include "../window.hpp"

namespace krypton::rapi {
    namespace vulkan {
        struct RenderObject {
            std::shared_ptr<krypton::mesh::Mesh> mesh;
            carbon::Buffer vertexBuffer;
            carbon::Buffer indexBuffer;
            uint64_t vertexCount = 0, indexCount = 0;
            std::vector<uint64_t> bufferVertexOffsets;
            std::vector<uint64_t> bufferIndexOffsets;

            RenderObject(const carbon::Context& ctx, std::shared_ptr<krypton::mesh::Mesh> mesh);
        };
    }

    class VulkanRT_RAPI final : public RenderAPI {
        krypton::rapi::Window window;
        std::shared_ptr<carbon::Context> ctx;
        std::unique_ptr<carbon::Swapchain> swapchain;
        std::vector<std::string> instanceExtensions;

        bool needsResize = false;

        std::vector<krypton::rapi::vulkan::RenderObject> objects = {};

    public:
        VulkanRT_RAPI();
        ~VulkanRT_RAPI();

        void drawFrame();
        krypton::rapi::Window* getWindow();
        void init();
        void render(std::shared_ptr<krypton::mesh::Mesh> mesh);
        void resize(int width, int height);
        void shutdown();
    };
}

#endif // #ifdef RAPI_WITH_VULKAN
