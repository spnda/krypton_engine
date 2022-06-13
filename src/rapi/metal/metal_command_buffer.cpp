#include <Tracy.hpp>

#include <rapi/backend_metal.hpp>
#include <rapi/metal/metal_buffer.hpp>
#include <rapi/metal/metal_command_buffer.hpp>
#include <rapi/metal/metal_renderpass.hpp>
#include <rapi/metal/metal_sampler.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <rapi/render_pass_attachments.hpp>
#include <rapi/vertex_descriptor.hpp>
#include <util/assert.hpp>
#include <util/handle.hpp>

namespace kr = krypton::rapi;
namespace ku = krypton::util;

kr::metal::CommandBuffer::~CommandBuffer() noexcept = default;

void kr::metal::CommandBuffer::beginRenderPass(const IRenderPass* handle) {
    ZoneScoped;
    auto* mtlRenderPass = dynamic_cast<const metal::RenderPass*>(handle);

    // Update texture handles
    for (auto& attachment : mtlRenderPass->attachments) {
        // Search for the swapchain texture handle
        auto* colorAttachment = mtlRenderPass->descriptor->colorAttachments()->object(attachment.first);
        if (attachment.second.attachment.get() == rapi->renderTargetTexture.get()) {
            colorAttachment->setTexture(drawable->texture());
            break;
        } else {
            colorAttachment->setTexture((dynamic_cast<metal::Texture*>(attachment.second.attachment.get()))->texture);
        }
    }

    curRenderEncoder = buffer->renderCommandEncoder(mtlRenderPass->descriptor);
    curRenderEncoder->setRenderPipelineState(mtlRenderPass->pipelineState);
}

void kr::metal::CommandBuffer::bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) {
    ZoneScoped;
    auto* mtlParameter = dynamic_cast<metal::ShaderParameter*>(parameter);

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

void kr::metal::CommandBuffer::bindVertexBuffer(IBuffer* vertexBuffer, std::size_t offset) {
    ZoneScoped;
    auto* mtlBuffer = dynamic_cast<metal::Buffer*>(vertexBuffer);

    curRenderEncoder->setVertexBuffer(mtlBuffer->buffer, offset, 0);
}

void kr::metal::CommandBuffer::drawIndexed(IBuffer* indexBuffer, uint32_t indexCount, IndexType type, uint32_t offset) {
    ZoneScoped;
    VERIFY(indexCount != 0);

    auto* metalBuffer = dynamic_cast<metal::Buffer*>(indexBuffer);

    curRenderEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, indexCount,
                                            type == IndexType::UINT16 ? MTL::IndexTypeUInt16 : MTL::IndexTypeUInt32, metalBuffer->buffer,
                                            offset);
}

void kr::metal::CommandBuffer::endRenderPass() {
    ZoneScoped;
    curRenderEncoder->endEncoding();
}

void kr::metal::CommandBuffer::presentFrame() {
    ZoneScoped;
    if (drawable != nullptr)
        buffer->presentDrawable(drawable);
}

void kr::metal::CommandBuffer::scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    ZoneScoped;
    MTL::ScissorRect scissor = {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
    };
    curRenderEncoder->setScissorRect(scissor);
}

void kr::metal::CommandBuffer::submit() {
    ZoneScoped;
    buffer->commit();
}

void kr::metal::CommandBuffer::viewport(float originX, float originY, float width, float height, float near, float far) {
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
