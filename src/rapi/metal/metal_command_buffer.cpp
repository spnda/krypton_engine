#include <Tracy.hpp>

#include <rapi/metal/metal_buffer.hpp>
#include <rapi/metal/metal_command_buffer.hpp>
#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_pipeline.hpp>
#include <rapi/metal/metal_queue.hpp>
#include <rapi/metal/metal_renderpass.hpp>
#include <rapi/metal/metal_sampler.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_shaderparameter.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <rapi/render_pass_attachments.hpp>
#include <util/assert.hpp>
#include <util/bits.hpp>

namespace kr = krypton::rapi;

#pragma region mtl::CommandBuffer
kr::mtl::CommandBuffer::CommandBuffer(MTL::Device* device, MTL::CommandQueue* commandQueue) : device(device), queue(commandQueue) {}

kr::mtl::CommandBuffer::~CommandBuffer() noexcept = default;

void kr::mtl::CommandBuffer::begin() {
    ZoneScoped;
    // Vulkan command buffers can be reused, while Metal command buffers can't.
    // We want to have that optimization available, so we'll just allocate a new MTLCommandBuffer
    // when beginning a new command buffer.
    buffer = queue->commandBuffer();
}

void kr::mtl::CommandBuffer::beginRenderPass(const IRenderPass* handle) {
    ZoneScoped;
    const auto* mtlRenderPass = dynamic_cast<const mtl::RenderPass*>(handle);

    // Update texture handles
    for (const auto& attachment : mtlRenderPass->attachments) {
        // Search for the swapchain texture handle
        auto* colorAttachment = mtlRenderPass->descriptor->colorAttachments()->object(attachment.first);
        colorAttachment->setTexture(dynamic_cast<Texture*>(attachment.second.attachment)->texture);
    }

    curRenderEncoder = buffer->renderCommandEncoder(mtlRenderPass->descriptor);
}

void kr::mtl::CommandBuffer::bindIndexBuffer(IBuffer* indexBuffer, IndexType type, uint32_t offset) {
    ZoneScoped;
    VERIFY(type != IndexType::UINT8); // Metal does not support single byte indices.
    boundIndexBuffer = dynamic_cast<mtl::Buffer*>(indexBuffer);
    boundIndexBufferOffset = offset;
    boundIndexType = type == IndexType::UINT16 ? MTL::IndexTypeUInt16 : MTL::IndexTypeUInt32;
}

void kr::mtl::CommandBuffer::bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) {
    ZoneScoped;
    auto* mtlParameter = dynamic_cast<mtl::ShaderParameter*>(parameter);
    auto indexOffset = boundPipeline->getUsesPushConstants() ? 1 : 0; // push constants occupy the first binding

    if (util::hasBit(stage, shaders::ShaderStage::Fragment)) {
        curRenderEncoder->setFragmentBuffer(mtlParameter->argumentBuffer, 0, index + indexOffset);
    }

    if (util::hasBit(stage, shaders::ShaderStage::Vertex)) {
        curRenderEncoder->setVertexBuffer(mtlParameter->argumentBuffer, 0, index + indexOffset);
    }

    auto mtlStages = getRenderStages(stage);
    for (auto& buf : mtlParameter->buffers) {
        curRenderEncoder->useResource(buf.second->buffer, MTL::ResourceUsageRead | MTL::ResourceUsageWrite, mtlStages);
    }

    for (auto& tex : mtlParameter->textures) {
        curRenderEncoder->useResource(tex.second->texture, MTL::ResourceUsageSample, mtlStages);
    }
}

void kr::mtl::CommandBuffer::bindPipeline(IPipeline* pipeline) {
    ZoneScoped;
    boundPipeline = dynamic_cast<Pipeline*>(pipeline);
    curRenderEncoder->setRenderPipelineState(boundPipeline->getState());
}

void kr::mtl::CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t firstIndex) {
    ZoneScoped;
    VERIFY(indexCount != 0);
    VERIFY(boundIndexBuffer != nullptr);

    auto indexSize = boundIndexType == MTL::IndexTypeUInt16 ? 2 : 4;
    curRenderEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, indexCount, boundIndexType, boundIndexBuffer->buffer,
                                            boundIndexBufferOffset + (firstIndex * indexSize));
}

void kr::mtl::CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t firstIndex, uint32_t instanceCount, uint32_t firstInstance) {
    ZoneScoped;
    VERIFY(indexCount != 0);
    VERIFY(boundIndexBuffer != nullptr);

    auto indexSize = boundIndexType == MTL::IndexTypeUInt16 ? 2 : 4;
    curRenderEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, indexCount, boundIndexType, boundIndexBuffer->buffer,
                                            boundIndexBufferOffset + (firstIndex * indexSize), instanceCount, 0, firstInstance);
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

void kr::mtl::CommandBuffer::pushConstants(uint32_t size, const void* data, shaders::ShaderStage stages) {
    ZoneScoped;
    if (util::hasBit(stages, shaders::ShaderStage::Fragment)) {
        curRenderEncoder->setFragmentBytes(data, size, 0);
    }
    if (util::hasBit(stages, shaders::ShaderStage::Vertex)) {
        curRenderEncoder->setVertexBytes(data, size, 0);
    }
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
        ret[count - i] = std::make_shared<CommandBuffer>(device, queue->getHandle());
    return ret;
}

void kr::mtl::CommandBufferPool::setName(std::string_view newName) {
    ZoneScoped;
    // Metal doesn't have a command pool... we can't name it.
}
#pragma endregion
