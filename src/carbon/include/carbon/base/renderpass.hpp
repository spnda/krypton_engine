#pragma once

#include <memory>
#include <string>
#include <vector>

#include <carbon/vulkan.hpp>

namespace carbon {
    // fwd
    class Device;
    class Swapchain;

    class RenderPass {
        std::shared_ptr<carbon::Device> device;
        std::shared_ptr<carbon::Swapchain> swapchain;

        VkRenderPass handle = VK_NULL_HANDLE;

    public:
        RenderPass(std::shared_ptr<carbon::Device> device, std::shared_ptr<carbon::Swapchain> swapchain);

        void create(VkAttachmentLoadOp colorBufferLoadOp, const std::string& name = {});
        void destroy();
        void begin(VkCommandBuffer cmdBuffer, VkFramebuffer framebuffer, std::vector<VkClearValue> clearValues = {});
        void end(VkCommandBuffer cmdBuffer);

        explicit operator VkRenderPass() const;
    };
} // namespace carbon
