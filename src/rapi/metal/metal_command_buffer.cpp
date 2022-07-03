#include <Tracy.hpp>

#include <rapi/metal/metal_buffer.hpp>
#include <rapi/metal/metal_command_buffer.hpp>
#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_queue.hpp>
#include <rapi/metal/metal_renderpass.hpp>
#include <rapi/metal/metal_sampler.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <rapi/render_pass_attachments.hpp>
#include <rapi/vertex_descriptor.hpp>
#include <shaders/shader_types.hpp>
#include <util/assert.hpp>

namespace kr = krypton::rapi;
namespace ku = krypton::util;

#pragma region mtl::CommandBuffer
kr::mtl::CommandBuffer::CommandBuffer(MTL::Device* device, MTL::CommandBuffer* commandBuffer) : device(device), buffer(commandBuffer) {}

kr::mtl::CommandBuffer::~CommandBuffer() noexcept = default;

void kr::mtl::CommandBuffer::begin() {
    // No need to begin Metal command buffers.
}

void kr::mtl::CommandBuffer::beginRenderPass(const IRenderPass* handle) {
    ZoneScoped;
    auto* mtlRenderPass = dynamic_cast<const mtl::RenderPass*>(handle);

    // Update texture handles
    for (auto& attachment : mtlRenderPass->attachments) {
        // Search for the swapchain texture handle
        auto* colorAttachment = mtlRenderPass->descriptor->colorAttachments()->object(attachment.first);
        /*if (attachment.second.attachment.get() == rapi->renderTargetTexture.get()) {
            colorAttachment->setTexture(drawable->texture());
            break;
        } else*/
        { colorAttachment->setTexture((dynamic_cast<mtl::Texture*>(attachment.second.attachment.get()))->texture); }
    }

    curRenderEncoder = buffer->renderCommandEncoder(mtlRenderPass->descriptor);
    curRenderEncoder->setRenderPipelineState(mtlRenderPass->pipelineState);
}

void kr::mtl::CommandBuffer::bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) {
    ZoneScoped;
    auto* mtlParameter = dynamic_cast<mtl::ShaderParameter*>(parameter);

    if ((stage & shaders::ShaderStage::Fragment) == shaders::ShaderStage::Fragment) {
        curRenderEncoder->setFragmentBuffer(mtlParameter->argumentBuffer, 0, index);
    }

    if ((stage & shaders::ShaderStage::Vertex) == shaders::ShaderStage::Vertex) {
        curRenderEncoder->setVertexBuffer(mtlParameter->argumentBuffer, 0, index);
    }

    for (auto& buf : mtlParameter->buffers) {
        // TODO: Pass the shaderStage parameter to the useResource call.
        curRenderEncoder->useResource(buf.second->buffer, MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
    }

    for (auto& tex : mtlParameter->textures) {
        curRenderEncoder->useResource(tex.second->texture, MTL::ResourceUsageSample);
    }
}

void kr::mtl::CommandBuffer::bindVertexBuffer(IBuffer* vertexBuffer, std::size_t offset) {
    ZoneScoped;
    auto* mtlBuffer = dynamic_cast<mtl::Buffer*>(vertexBuffer);

    curRenderEncoder->setVertexBuffer(mtlBuffer->buffer, offset, 0);
}

void kr::mtl::CommandBuffer::drawIndexed(IBuffer* indexBuffer, uint32_t indexCount, IndexType type, uint32_t offset) {
    ZoneScoped;
    VERIFY(indexCount != 0);

    auto* metalBuffer = dynamic_cast<mtl::Buffer*>(indexBuffer);

    curRenderEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, indexCount,
                                            type == IndexType::UINT16 ? MTL::IndexTypeUInt16 : MTL::IndexTypeUInt32, metalBuffer->buffer,
                                            offset);
}

void kr::mtl::CommandBuffer::end() {}

void kr::mtl::CommandBuffer::endRenderPass() {
    ZoneScoped;
    curRenderEncoder->endEncoding();
}

void kr::mtl::CommandBuffer::setName(std::string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    if (buffer != nullptr)
        buffer->setLabel(name);
}

void kr::mtl::CommandBuffer::scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    ZoneScoped;
    MTL::ScissorRect scissor = {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
    };
    curRenderEncoder->setScissorRect(scissor);
}

void kr::mtl::CommandBuffer::viewport(float originX, float originY, float width, float height, float near, float far) {
    ZoneScoped;
    MTL::Viewport viewport = {
        .originX = originX,
        .originY = originY,
        .width = width,
        .height = height,
        .znear = near,
        .zfar = far,
    };
    curRenderEncoder->setViewport(viewport);
}
#pragma endregion

#pragma region mtl::CommandBufferPool
kr::mtl::CommandBufferPool::CommandBufferPool(MTL::Device* device, Queue* queue) : device(device), queue(queue) {}

std::vector<std::shared_ptr<kr::ICommandBuffer>> kr::mtl::CommandBufferPool::allocateCommandBuffers(uint32_t count) {
    ZoneScoped;
    std::vector<std::shared_ptr<kr::ICommandBuffer>> ret(count);
    for (auto i = count; i > 0; --i)
        ret[count - i] = std::make_shared<CommandBuffer>(device, queue->getHandle()->commandBuffer());
    return ret;
}

void kr::mtl::CommandBufferPool::setName(std::string_view newName) {
    ZoneScoped;
    // Metal doesn't have a command pool... we can't name it.
}
#pragma endregion
