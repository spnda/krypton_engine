#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_command_buffer.hpp>
#include <rapi/vulkan/vk_device.hpp>

namespace kr = krypton::rapi;

kr::vk::CommandBuffer::CommandBuffer(Device* device) : device(device) {}

void kr::vk::CommandBuffer::beginRenderPass(const IRenderPass* renderPass) {}

void kr::vk::CommandBuffer::bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) {}

void kr::vk::CommandBuffer::bindVertexBuffer(IBuffer* buffer, std::size_t offset) {}

void kr::vk::CommandBuffer::drawIndexed(IBuffer* indexBuffer, uint32_t indexCount, IndexType type, uint32_t offset) {}

void kr::vk::CommandBuffer::endRenderPass() {}

void kr::vk::CommandBuffer::presentFrame() {}

void kr::vk::CommandBuffer::setName(std::string_view name) {}

void kr::vk::CommandBuffer::scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {}

void kr::vk::CommandBuffer::submit() {}

void kr::vk::CommandBuffer::viewport(float originX, float originY, float width, float height, float near, float far) {}
