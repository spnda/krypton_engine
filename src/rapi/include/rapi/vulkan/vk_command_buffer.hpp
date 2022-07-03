#pragma once

#include <rapi/icommandbuffer.hpp>

// fwd.
typedef struct VkCommandBuffer_T* VkCommandBuffer;
typedef struct VkCommandPool_T* VkCommandPool;

namespace krypton::rapi::vk {
    class CommandBuffer final : public ICommandBuffer {
        class Device* device;
        class Queue* queue;

        VkCommandBuffer cmdBuffer;

    public:
        explicit CommandBuffer(Device* device, Queue* queue, VkCommandBuffer buffer);
        ~CommandBuffer() override = default;

        void begin() override;
        void beginRenderPass(const IRenderPass* renderPass) override;
        void bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) override;
        void bindVertexBuffer(IBuffer* buffer, std::size_t offset) override;
        void drawIndexed(IBuffer* indexBuffer, uint32_t indexCount, IndexType type, uint32_t offset) override;
        void end() override;
        void endRenderPass() override;
        [[nodiscard]] auto getHandle() const -> VkCommandBuffer;
        void setName(std::string_view name) override;
        void scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
        void viewport(float originX, float originY, float width, float height, float near, float far) override;
    };

    class CommandBufferPool final : public ICommandBufferPool {
        class Device* device;
        class Queue* queue;

        VkCommandPool commandPool;
        std::string name;

    public:
        explicit CommandBufferPool(Device* device, Queue*, VkCommandPool pool);
        ~CommandBufferPool() override = default;

        auto allocateCommandBuffers(uint32_t count) -> std::vector<std::shared_ptr<ICommandBuffer>> override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::vk