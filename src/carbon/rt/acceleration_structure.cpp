#include "acceleration_structure.hpp"

#include "../context.hpp"
#include "../resource/stagingbuffer.hpp"

carbon::AccelerationStructure::AccelerationStructure(const carbon::Context& context, carbon::AccelerationStructureType asType, const std::string name)
        : ctx(context), type(asType),
        resultBuffer(ctx, std::move(name)), scratchBuffer(ctx, std::move(name)) {
}

void carbon::AccelerationStructure::createScratchBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizes) {
    scratchBuffer.create(
        buildSizes.buildScratchSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
}

void carbon::AccelerationStructure::createResultBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizes) {
    resultBuffer.create(
        buildSizes.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
}

void carbon::AccelerationStructure::createStructure(VkAccelerationStructureBuildSizesInfoKHR buildSizes) {
    VkAccelerationStructureCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .buffer = resultBuffer.getHandle(),
        .size = buildSizes.accelerationStructureSize,
        .type = static_cast<VkAccelerationStructureTypeKHR>(type),
    };
    ctx.createAccelerationStructure(createInfo, &handle);
    address = ctx.getAccelerationStructureDeviceAddress(handle);
}

void carbon::AccelerationStructure::destroy() {
    ctx.destroyAccelerationStructure(handle);
    resultBuffer.destroy();
    scratchBuffer.destroy();
}

VkAccelerationStructureBuildSizesInfoKHR carbon::AccelerationStructure::getBuildSizes(const uint32_t* primitiveCount,
                                                                                  VkAccelerationStructureBuildGeometryInfoKHR* buildGeometryInfo,
                                                                                  VkPhysicalDeviceAccelerationStructurePropertiesKHR asProperties) {
    VkAccelerationStructureBuildSizesInfoKHR buildSizes = ctx.getAccelerationStructureBuildSizes(primitiveCount, buildGeometryInfo);
    buildSizes.accelerationStructureSize = carbon::Buffer::alignedSize(buildSizes.accelerationStructureSize, 256); // Apparently, this is part of the Vulkan Spec
    buildSizes.buildScratchSize = carbon::Buffer::alignedSize(buildSizes.buildScratchSize, asProperties.minAccelerationStructureScratchOffsetAlignment);
    return buildSizes;
}

VkWriteDescriptorSetAccelerationStructureKHR carbon::AccelerationStructure::getDescriptorWrite() const {
    return {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .pNext = nullptr,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &handle,
    };
}

carbon::BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(const carbon::Context& context, const std::string& name)
        : AccelerationStructure(context, carbon::AccelerationStructureType::BottomLevel, name),
          vertexBuffer(ctx, "vertexBuffer"),
          indexBuffer(ctx, "indexBuffer"),
          transformBuffer(ctx, "transformBuffer"),
          vertexStagingBuffer(ctx, "vertexStagingBuffer"),
          indexStagingBuffer(ctx, "indexStagingBuffer"),
          transformStagingBuffer(ctx, "transformStagingBuffer") {
}

void carbon::BottomLevelAccelerationStructure::createMeshBuffers(std::vector<PrimitiveData> primitiveData, VkTransformMatrixKHR transform) {
    // We want to squash every primitive of the mesh into a single long buffer.
    // This will make the buffer quite convoluted, as there are vertices and indices
    // over and over again, but it should help for simplicity’s sake.

    // First, we compute the total size of our mesh buffer.
    uint64_t totalVertexSize = 0, totalIndexSize = 0;
    for (const auto& prim : primitiveData) {
        totalVertexSize += prim.first.size();
        totalIndexSize += prim.second.size();
    }

    vertexStagingBuffer.create(totalVertexSize);
    indexStagingBuffer.create(totalIndexSize);
    transformStagingBuffer.create(sizeof(VkTransformMatrixKHR));

    // Copy the data into the big buffer at an offset.
    uint64_t currentVertexOffset = 0, currentIndexOffset = 0;
    for (const auto& prim : primitiveData) {
        uint64_t vertexSize = prim.first.size();
        uint64_t indexSize = prim.second.size();

        vertexStagingBuffer.memoryCopy(prim.first.data(), vertexSize, currentVertexOffset);
        indexStagingBuffer.memoryCopy(prim.second.data(), indexSize, currentIndexOffset);

        currentVertexOffset += vertexSize;
        currentIndexOffset += indexSize;
    }

    transformStagingBuffer.memoryCopy(&transform, sizeof(VkTransformMatrixKHR));

    // At last, we create the real buffers that reside on the GPU.
    VkBufferUsageFlags asInputBufferUsage =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    vertexBuffer.create(totalVertexSize, asInputBufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);
    indexBuffer.create(totalIndexSize, asInputBufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);
    transformBuffer.create(sizeof(VkTransformMatrixKHR), asInputBufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);
}

void carbon::BottomLevelAccelerationStructure::copyMeshBuffers(VkCommandBuffer const cmdBuffer) {
    vertexStagingBuffer.copyToBuffer(cmdBuffer, vertexBuffer);
    indexStagingBuffer.copyToBuffer(cmdBuffer, indexBuffer);
    transformStagingBuffer.copyToBuffer(cmdBuffer, transformBuffer);
}

void carbon::BottomLevelAccelerationStructure::destroyMeshBuffers() {
    vertexStagingBuffer.destroy();
    indexStagingBuffer.destroy();
}

carbon::TopLevelAccelerationStructure::TopLevelAccelerationStructure(const carbon::Context& ctx)
        : AccelerationStructure(ctx, carbon::AccelerationStructureType::TopLevel, "tlas") {
}
