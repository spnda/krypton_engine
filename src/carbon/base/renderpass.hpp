#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace carbon {
    // fwd
    class Context;
    class Swapchain;

    class RenderPass {
        const carbon::Context& ctx;
        const carbon::Swapchain& swapchain;

        VkRenderPass handle = VK_NULL_HANDLE;

    public:
        RenderPass(const carbon::Context& context, const carbon::Swapchain& swapchain);

        void create(VkAttachmentLoadOp colorBufferLoadOp, const std::string& name = {});
        void destroy();
        void begin(VkCommandBuffer cmdBuffer, VkFramebuffer framebuffer, std::vector<VkClearValue> clearValues = {});
        void end(VkCommandBuffer cmdBuffer);

        explicit operator VkRenderPass() const;
    };
}
