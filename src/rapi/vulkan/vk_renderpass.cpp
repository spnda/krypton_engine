#include <array>

#include <Tracy.hpp>
#include <volk.h>

#include <rapi/render_pass_attachments.hpp>
#include <rapi/vulkan/vk_renderpass.hpp>
#include <rapi/vulkan/vk_texture.hpp>

namespace kr = krypton::rapi;

// clang-format off
static constexpr std::array<VkAttachmentLoadOp, 3> vulkanLoadActions = {
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,    // AttachmentLoadAction::DontCare = 0,
    VK_ATTACHMENT_LOAD_OP_LOAD,         // AttachmentLoadAction::Load = 1,
    VK_ATTACHMENT_LOAD_OP_CLEAR,        // AttachmentLoadAction::Clear = 2,
};

static constexpr std::array<VkAttachmentStoreOp, 3> vulkanStoreActions = {
    VK_ATTACHMENT_STORE_OP_DONT_CARE,   // AttachmentStoreAction::DontCare = 0,
    VK_ATTACHMENT_STORE_OP_STORE,       // AttachmentStoreAction::Store = 1,
    VK_ATTACHMENT_STORE_OP_STORE,       // AttachmentStoreAction::Multisample = 2,
};
// clang-format on

kr::vk::RenderPass::RenderPass(Device* device) : device(device) {}

void kr::vk::RenderPass::setAttachment(uint32_t index, RenderPassAttachment attachment) {
    ZoneScoped;
    attachments[index] = attachment;
}

kr::RenderPassAttachment& kr::vk::RenderPass::getAttachment(uint32_t index) {
    ZoneScoped;
    return attachments[index];
}

void kr::vk::RenderPass::build() {
    ZoneScoped;
    attachmentInfos.resize(attachments.size());
    for (auto& attachment : attachments) {
        auto& second = attachment.second;

        if (second.attachment == nullptr) {
            attachmentInfos[attachment.first] = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            };
            continue;
        }

        auto vkTexture = dynamic_cast<Texture*>(second.attachment);
        VkClearValue clearValue;
        std::memcpy(&clearValue.color, &second.clearColor, sizeof(second.clearColor));
        attachmentInfos[attachment.first] = VkRenderingAttachmentInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = vkTexture->getView(),
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = vulkanLoadActions[static_cast<uint8_t>(second.loadAction)],
            .storeOp = vulkanStoreActions[static_cast<uint8_t>(second.storeAction)],
            .clearValue = clearValue,
        };
    }

    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(attachmentInfos.size());
    renderingInfo.pColorAttachments = attachmentInfos.data();
}

void kr::vk::RenderPass::destroy() {
    ZoneScoped;
}

const VkRenderingInfo* kr::vk::RenderPass::getRenderingInfo() const noexcept {
    return &renderingInfo;
}
