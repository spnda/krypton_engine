#pragma once

#include <optional>
#include <vector>

#include <robin_hood.h>

#include <rapi/irenderpass.hpp>
#include <rapi/render_pass_attachments.hpp>

namespace krypton::rapi::vk {
    class RenderPass final : public IRenderPass {
        class Device* device;

        robin_hood::unordered_flat_map<uint32_t, RenderPassAttachment> attachments = {};
        std::optional<RenderPassDepthAttachment> depthAttachment = {};

        std::vector<VkRenderingAttachmentInfo> attachmentInfos;
        VkRenderingInfo renderingInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .layerCount = 1,
        };

    public:
        explicit RenderPass(Device* device);

        void setAttachment(uint32_t index, RenderPassAttachment attachment) override;
        auto getAttachment(uint32_t index) -> RenderPassAttachment& override;
        void build() override;
        void destroy() override;
        [[nodiscard]] auto getRenderingInfo() const noexcept -> const VkRenderingInfo*;
    };
} // namespace krypton::rapi::vk
