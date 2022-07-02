#pragma once

#include <rapi/icommandbuffer.hpp>

// fwd.
typedef struct VkCommandBuffer_T* VkCommandBuffer;

namespace krypton::rapi::vk {
    class CommandBuffer : public ICommandBuffer {
        class Device* device;

        VkCommandBuffer cmdBuffer = nullptr;

    public:
        explicit CommandBuffer(Device* device);
        ~CommandBuffer() override = default;

        void beginRenderPass(const IRenderPass* renderPass) override;
        void bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) override;
        void bindVertexBuffer(IBuffer* buffer, std::size_t offset) override;
        void drawIndexed(IBuffer* indexBuffer, uint32_t indexCount, IndexType type, uint32_t offset) override;
        void endRenderPass() override;
        void presentFrame() override;
        void setName(std::string_view name) override;
        void scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
        void submit() override;
        void viewport(float originX, float originY, float width, float height, float near, float far) override;
    };
} // namespace krypton::rapi::vk