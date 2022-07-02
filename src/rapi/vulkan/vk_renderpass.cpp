#include <Tracy.hpp>
#include <volk.h>

#include <rapi/render_pass_attachments.hpp>
#include <rapi/vertex_descriptor.hpp>
#include <rapi/vulkan/vk_renderpass.hpp>

namespace kr = krypton::rapi;

kr::vk::RenderPass::RenderPass(Device* device) : device(device) {}

void kr::vk::RenderPass::addAttachment(uint32_t index, RenderPassAttachment attachment) {
    ZoneScoped;
}

void kr::vk::RenderPass::build() {
    ZoneScoped;
}

void kr::vk::RenderPass::destroy() {
    ZoneScoped;
}

void kr::vk::RenderPass::setFragmentFunction(const IShader* shader) {}

void kr::vk::RenderPass::setVertexDescriptor(VertexDescriptor descriptor) {}

void kr::vk::RenderPass::setVertexFunction(const IShader* shader) {}
