#include <carbon/base/command_buffer.hpp>
#include <carbon/base/device.hpp>
#include <carbon/resource/stagingbuffer.hpp>
#include <carbon/rt/acceleration_structure.hpp>

carbon::AccelerationStructure::AccelerationStructure(std::shared_ptr<carbon::Device> device, VmaAllocator allocator,
                                                     carbon::AccelerationStructureType asType, std::string name)
    : device(std::move(device)), allocator(allocator), type(asType), name(std::move(name)) {}

void carbon::AccelerationStructure::createScratchBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizes) {
    scratchBuffer = std::make_shared<carbon::Buffer>(this->device, allocator, name);

    scratchBuffer->create(buildSizes.buildScratchSize,
                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void carbon::AccelerationStructure::createResultBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizes) {
    resultBuffer = std::make_shared<carbon::Buffer>(this->device, allocator, name);

    resultBuffer->create(buildSizes.accelerationStructureSize,
                         VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                         VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void carbon::AccelerationStructure::createStructure(VkAccelerationStructureBuildSizesInfoKHR buildSizes) {
    mutex.lock();
    VkAccelerationStructureCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .buffer = resultBuffer->getHandle(),
        .size = buildSizes.accelerationStructureSize,
        .type = static_cast<VkAccelerationStructureTypeKHR>(type),
    };
    device->vkCreateAccelerationStructureKHR(*device, &createInfo, nullptr, &handle);

    VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = handle,
    };
    address = device->vkGetAccelerationStructureDeviceAddressKHR(*device, &addressInfo);
    mutex.unlock();
}

void carbon::AccelerationStructure::destroy() {
    mutex.lock();
    device->vkDestroyAccelerationStructureKHR(*device, handle, nullptr);
    if (resultBuffer != nullptr)
        resultBuffer->destroy();
    if (scratchBuffer != nullptr)
        scratchBuffer->destroy();
    mutex.unlock();
}

VkAccelerationStructureBuildSizesInfoKHR
carbon::AccelerationStructure::getBuildSizes(const uint32_t* primitiveCount, VkAccelerationStructureBuildGeometryInfoKHR* buildGeometryInfo,
                                             VkPhysicalDeviceAccelerationStructurePropertiesKHR asProperties) {
    VkAccelerationStructureBuildSizesInfoKHR buildSizes = {};
    buildSizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    device->vkGetAccelerationStructureBuildSizesKHR(*device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, buildGeometryInfo,
                                                    primitiveCount, &buildSizes);

    buildSizes.accelerationStructureSize = carbon::Buffer::alignedSize(
        buildSizes.accelerationStructureSize, static_cast<uint32_t>(256)); // Apparently, this is part of the Vulkan Spec
    buildSizes.buildScratchSize =
        carbon::Buffer::alignedSize(buildSizes.buildScratchSize, asProperties.minAccelerationStructureScratchOffsetAlignment);
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

carbon::AccelerationStructure::operator bool() const noexcept {
    auto guard = std::unique_lock(mutex);
    guard.lock();
    return handle != nullptr;
}

carbon::BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(std::shared_ptr<carbon::Device> device, VmaAllocator allocator,
                                                                           const std::string& name)
    : AccelerationStructure(device, allocator, carbon::AccelerationStructureType::BottomLevel, name) {
    vertexStagingBuffer = std::make_unique<carbon::StagingBuffer>(device, allocator, "vertexStagingBuffer");
    indexStagingBuffer = std::make_unique<carbon::StagingBuffer>(device, allocator, "indexStagingBuffer");
    transformStagingBuffer = std::make_unique<carbon::StagingBuffer>(device, allocator, "transformStagingBuffer");

    vertexBuffer = std::make_unique<carbon::Buffer>(device, allocator, "vertexBuffer");
    indexBuffer = std::make_unique<carbon::Buffer>(device, allocator, "indexBuffer");
    transformBuffer = std::make_unique<carbon::Buffer>(device, allocator, "transformBuffer");
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

    vertexStagingBuffer->create(totalVertexSize);
    indexStagingBuffer->create(totalIndexSize);
    transformStagingBuffer->create(sizeof(VkTransformMatrixKHR));

    // Copy the data into the big buffer at an offset.
    uint64_t currentVertexOffset = 0, currentIndexOffset = 0;
    for (const auto& prim : primitiveData) {
        uint64_t vertexSize = prim.first.size();
        uint64_t indexSize = prim.second.size();

        vertexStagingBuffer->memoryCopy(prim.first.data(), vertexSize, currentVertexOffset);
        indexStagingBuffer->memoryCopy(prim.second.data(), indexSize, currentIndexOffset);

        currentVertexOffset += vertexSize;
        currentIndexOffset += indexSize;
    }

    transformStagingBuffer->memoryCopy(&transform, sizeof(VkTransformMatrixKHR));

    // At last, we create the real buffers that reside on the GPU.
    VkBufferUsageFlags asInputBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    vertexBuffer->create(totalVertexSize, asInputBufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);
    indexBuffer->create(totalIndexSize, asInputBufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);
    transformBuffer->create(sizeof(VkTransformMatrixKHR), asInputBufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);
}

void carbon::BottomLevelAccelerationStructure::copyMeshBuffers(carbon::CommandBuffer* cmdBuffer) {
    vertexStagingBuffer->copyToBuffer(cmdBuffer, vertexBuffer.get());
    indexStagingBuffer->copyToBuffer(cmdBuffer, indexBuffer.get());
    transformStagingBuffer->copyToBuffer(cmdBuffer, transformBuffer.get());
}

void carbon::BottomLevelAccelerationStructure::destroyMeshBuffers() {
    vertexStagingBuffer->destroy();
    indexStagingBuffer->destroy();
    transformStagingBuffer->destroy();
}

void carbon::BottomLevelAccelerationStructure::destroy() {
    vertexBuffer->destroy();
    indexBuffer->destroy();
    transformBuffer->destroy();
    carbon::AccelerationStructure::destroy(); /* Call base */
}

carbon::TopLevelAccelerationStructure::TopLevelAccelerationStructure(std::shared_ptr<carbon::Device> device, VmaAllocator allocator)
    : AccelerationStructure(device, allocator, carbon::AccelerationStructureType::TopLevel, "tlas") {}
