#ifdef RAPI_WITH_VULKAN

#include <algorithm>
#include <cctype>
#include <iostream>
#include <locale>

#include <fmt/core.h>

#include <carbon/base/command_buffer.hpp>
#include <carbon/base/command_pool.hpp>
#include <carbon/base/device.hpp>
#include <carbon/base/fence.hpp>
#include <carbon/base/instance.hpp>
#include <carbon/base/physical_device.hpp>
#include <carbon/base/semaphore.hpp>
#include <carbon/base/swapchain.hpp>
#include <carbon/resource/buffer.hpp>
#include <carbon/resource/image.hpp>
#include <carbon/resource/stagingbuffer.hpp>
#include <carbon/resource/storageimage.hpp>
#include <carbon/rt/acceleration_structure.hpp>
#include <carbon/rt/rt_pipeline.hpp>
#include <carbon/shaders/shader.hpp>
#include <carbon/utils.hpp>
#include <carbon/vulkan.hpp>

#include <rapi/backends/vulkan_rt_backend.hpp>
#include <rapi/render_object_handle.hpp>

krypton::rapi::VulkanRT_RAPI::VulkanRT_RAPI() {
    instance = std::make_shared<carbon::Instance>();
    instance->setApplicationData({.apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0),
                                  .applicationVersion = 1,
                                  .engineVersion = 1,

                                  .applicationName = "Krypton",
                                  .engineName = "Krypton Engine"});

    physicalDevice = std::make_shared<carbon::PhysicalDevice>();
    device = std::make_shared<carbon::Device>();
    window = std::make_shared<krypton::rapi::Window>("Krypton", 1920, 1080);

    commandPool = std::make_shared<carbon::CommandPool>(device);

    renderFence = std::make_shared<carbon::Fence>(device, "renderFence");
    presentCompleteSemaphore = std::make_shared<carbon::Semaphore>(device, "presentComplete");
    renderCompleteSemaphore = std::make_shared<carbon::Semaphore>(device, "renderComplete");

    swapchain = std::make_shared<carbon::Swapchain>(instance, device);
    graphicsQueue = std::make_shared<carbon::Queue>(device, "graphicsQueue");
}

krypton::rapi::VulkanRT_RAPI::~VulkanRT_RAPI() {
    shutdown();
}

void krypton::rapi::VulkanRT_RAPI::beginFrame() {
    if (!needsResize) {
        auto result = waitForFrame();
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            needsResize = true;
        } else {
            checkResult(graphicsQueue.get(), result, "Failed to present queue!");
        }
    }
}

void krypton::rapi::VulkanRT_RAPI::buildBLAS(krypton::rapi::vulkan::RenderObject& renderObject) {
    auto primitiveSize = renderObject.mesh->primitives.size();
    std::vector<uint32_t> primitiveCounts(primitiveSize);
    std::vector<VkAccelerationStructureGeometryKHR> geometries(primitiveSize);
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfos(primitiveSize);

    /** Calculate the total buffer sizes and offsets */
    std::vector<carbon::BottomLevelAccelerationStructure::PrimitiveData> primitiveData(primitiveSize);
    uint64_t totalVertexSize = 0, totalIndexSize = 0;
    for (auto& prim : renderObject.mesh->primitives) {
        uint64_t primitiveIndex = &prim - &renderObject.mesh->primitives.front();

        auto& description = renderObject.geometryDescriptions.emplace_back();
        description.materialIndex = prim.materialIndex;
        description.meshBufferVertexOffset = totalVertexSize;
        description.meshBufferIndexOffset = totalIndexSize;

        std::span<std::byte> vertex = {reinterpret_cast<std::byte*>(prim.vertices.data()), prim.vertices.size() * krypton::mesh::VERTEX_STRIDE};
        std::span<std::byte> index = {reinterpret_cast<std::byte*>(prim.indices.data()), prim.indices.size() * sizeof(uint32_t)};

        totalVertexSize += vertex.size();
        totalIndexSize += index.size();

        primitiveData.push_back(std::make_pair(vertex, index));
    }

    renderObject.blas->createMeshBuffers(primitiveData,
                                         *reinterpret_cast<VkTransformMatrixKHR*>(&renderObject.mesh->transform));

    /** Generate the geometry data with the buffer info just generated */
    for (const auto& prim : renderObject.mesh->primitives) {
        uint64_t primitiveIndex = &prim - &renderObject.mesh->primitives.front();
        primitiveCounts[primitiveIndex] = std::min(prim.indices.size() / 3, (size_t)536870911); /* Specific value for my 2080 */

        geometries[primitiveIndex] = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            .geometry = {
                .triangles = {
                    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                    .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT, // TODO: Replace with variable
                    .vertexData {
                        .deviceAddress = renderObject.blas->vertexBuffer->getDeviceAddress() + renderObject.geometryDescriptions[primitiveIndex].meshBufferVertexOffset,
                    },
                    .vertexStride = krypton::mesh::VERTEX_STRIDE,
                    .maxVertex = static_cast<uint32_t>(prim.vertices.size() - 1),
                    .indexType = VK_INDEX_TYPE_UINT32,
                    .indexData = {
                        .deviceAddress = renderObject.blas->indexBuffer->getDeviceAddress() + renderObject.geometryDescriptions[primitiveIndex].meshBufferIndexOffset,
                    },
                    .transformData = {
                        .deviceAddress = renderObject.blas->transformBuffer->getDeviceAddress(),
                    },
                },
            }};

        rangeInfos[primitiveIndex] = {
            .primitiveCount = primitiveCounts[primitiveIndex],
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0,
        };
    }

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = static_cast<uint32_t>(geometries.size()),
        .pGeometries = geometries.data(),
    };

    /** Create the structure's memory */
    auto sizes = renderObject.blas->getBuildSizes(
        primitiveCounts.data(), &buildGeometryInfo,
        VkPhysicalDeviceAccelerationStructurePropertiesKHR {
            .minAccelerationStructureScratchOffsetAlignment = 128, /* Specific value for my 2080 */
        });
    renderObject.blas->createScratchBuffer(sizes);
    renderObject.blas->createResultBuffer(sizes);
    renderObject.blas->createStructure(sizes);
    device->setDebugUtilsName(renderObject.blas->handle, renderObject.mesh->name);

    buildGeometryInfo.dstAccelerationStructure = renderObject.blas->handle;
    buildGeometryInfo.scratchData.deviceAddress = renderObject.blas->scratchBuffer->getDeviceAddress();

    /** We're only building one BLAS here currently. */
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos = {buildGeometryInfo};
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> rangeInfoPtrs;
    rangeInfoPtrs.push_back(rangeInfos.data());

    /** Dispatch the commands */
    oneTimeSubmit(graphicsQueue.get(), commandPool.get(), [&](carbon::CommandBuffer* cmdBuffer) {
        /* First copy our mesh buffers */
        cmdBuffer->setCheckpoint("Copying mesh buffers!");
        renderObject.blas->copyMeshBuffers(cmdBuffer);

        /* Insert memory barrier for the buffer copies */
        VkMemoryBarrier memBarrier = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
        };
        cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0,
                                   1, &memBarrier, 0, nullptr, 0, nullptr);

        /* Build the AS */
        cmdBuffer->setCheckpoint("Building BLAS!");
        cmdBuffer->buildAccelerationStructures(buildGeometryInfos, rangeInfoPtrs);
    });

    renderObject.blas->destroyMeshBuffers();
}

void krypton::rapi::VulkanRT_RAPI::buildPipeline() {
    if (pipeline != nullptr && pipeline->pipeline != nullptr) {
        vkDestroyDescriptorSetLayout(*device, pipeline->descriptorLayout, nullptr);
        vkDestroyPipelineLayout(*device, pipeline->pipelineLayout, nullptr);
        vkDestroyPipeline(*device, pipeline->pipeline, nullptr);
    }

    auto builder = carbon::RayTracingPipelineBuilder::create(device, "rt_pipeline")
                       .addShaderGroup(carbon::RtShaderGroup::General, {*rayGenShader.shader})
                       .addShaderGroup(carbon::RtShaderGroup::General, {*missShader.shader})
                       .addShaderGroup(carbon::RtShaderGroup::TriangleHit, {*closestHitShader.shader, *anyHitShader.shader});

    /* Add descriptors */
    auto tlasDescriptor = tlas->getDescriptorWrite();
    builder.addAccelerationStructureDescriptor(
        0, &tlasDescriptor,
        carbon::ShaderStage::RayGeneration | carbon::ShaderStage::ClosestHit);

    VkDescriptorImageInfo storageImageDescriptor = storageImage->getDescriptorImageInfo();
    builder.addImageDescriptor(
        1, &storageImageDescriptor,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, carbon::ShaderStage::RayGeneration);

    VkDescriptorBufferInfo cameraBufferInfo = cameraBuffer->getDescriptorInfo(VK_WHOLE_SIZE);
    builder.addBufferDescriptor(
        2, &cameraBufferInfo,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, carbon::ShaderStage::RayGeneration);

    pipeline = std::move(builder.build());
}

void krypton::rapi::VulkanRT_RAPI::buildSBT() {
    uint32_t missCount = 1; // 1 miss groups
    uint32_t chitCount = 1; // 1 hit group (with closest and any)
    uint32_t callCount = 0;
    auto handleCount = 1 + missCount + chitCount + callCount;
    const uint32_t handleSize = 32;                          // Specific value for my 2080
    sbtStride = carbon::Buffer::alignedSize(handleSize, 32); // Specific value for my 2080

    rayGenShader.region.stride = carbon::Buffer::alignedSize(sbtStride, 64); // Specific value for my 2080
    rayGenShader.region.size = rayGenShader.region.stride;                   // RayGen size must be equal to the stride.
    missShader.region.stride = sbtStride;
    missShader.region.size = carbon::Buffer::alignedSize(missCount * sbtStride, 64); // Specific value for my 2080
    closestHitShader.region.stride = sbtStride;
    closestHitShader.region.size = carbon::Buffer::alignedSize(chitCount * sbtStride, 64); // Specific value for my 2080

    const uint32_t sbtSize = rayGenShader.region.size + missShader.region.size + closestHitShader.region.size;
    std::vector<uint8_t> handleStorage(sbtSize);
    auto result = device->vkGetRayTracingShaderGroupHandlesKHR(*device, pipeline->pipeline, 0, handleCount, sbtSize, handleStorage.data());
    checkResult(result, "Failed to get ray tracing shader group handles!");

    shaderBindingTable->create(sbtSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                               VMA_MEMORY_USAGE_CPU_TO_GPU);
    auto sbtAddress = shaderBindingTable->getDeviceAddress();
    rayGenShader.region.deviceAddress = sbtAddress;
    missShader.region.deviceAddress = sbtAddress + rayGenShader.region.size;
    closestHitShader.region.deviceAddress = missShader.region.deviceAddress + missShader.region.size;

    shaderBindingTable->memoryCopy(handleStorage.data(), sbtSize);

    auto getHandleOffset = [&](uint32_t i) -> auto {
        return handleStorage.data() + i * handleSize;
    };
    uint32_t curHandleIndex = 0;

    // Write raygen shader
    shaderBindingTable->memoryCopy(getHandleOffset(curHandleIndex++), handleSize, 0);

    // Write miss shaders
    for (uint32_t i = 0; i < missCount; i++) {
        shaderBindingTable->memoryCopy(
            getHandleOffset(curHandleIndex++),
            handleSize,
            rayGenShader.region.size + missShader.region.stride * i);
    }

    // Write chit shaders
    for (uint32_t i = 0; i < chitCount; i++) {
        shaderBindingTable->memoryCopy(
            getHandleOffset(curHandleIndex++),
            handleSize,
            rayGenShader.region.size + missShader.region.size + closestHitShader.region.stride * i);
    }
}

void krypton::rapi::VulkanRT_RAPI::buildTLAS(carbon::CommandBuffer* cmdBuffer) {
    const uint32_t primitiveCount = handlesForFrame.size();

    std::vector<VkAccelerationStructureInstanceKHR> instances(primitiveCount, VkAccelerationStructureInstanceKHR {});
    for (auto& handle : handlesForFrame) {
        size_t index = &handle - &handlesForFrame[0];
        // clang-format off
        instances[index].transform = {
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
        };
        // clang-format on
        instances[index].instanceCustomIndex = static_cast<uint32_t>(index);
        instances[index].mask = 0xFF;
        instances[index].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instances[index].instanceShaderBindingTableRecordOffset = 0;
        instances[index].accelerationStructureReference = objects.getFromHandle(handle).blas->address;
    }

    VkDeviceAddress instanceDataDeviceAddress = {};

    if (primitiveCount > 0) {
        size_t instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * primitiveCount;
        if (instanceBufferSize <= tlasInstanceStagingBuffer->getSize() && tlasInstanceStagingBuffer->getSize() != 0) {
            /* We can re-use the old instance buffers */
            tlasInstanceStagingBuffer->memoryCopy(instances.data(), instanceBufferSize);
            instanceDataDeviceAddress = tlasInstanceBuffer->getDeviceAddress(); /* Same address as before */
        } else {
            /* As we cannot use the old buffers, we'll destroy those now */
            tlasInstanceStagingBuffer->destroy();
            tlasInstanceBuffer->destroy();

            tlasInstanceStagingBuffer->create(instanceBufferSize);
            tlasInstanceStagingBuffer->memoryCopy(instances.data(), instanceBufferSize);

            tlasInstanceBuffer->create(instanceBufferSize,
                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                       VMA_MEMORY_USAGE_GPU_ONLY);
            instanceDataDeviceAddress = tlasInstanceBuffer->getDeviceAddress();
        }
    }

    VkAccelerationStructureGeometryKHR geometry = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry = {
            .instances = {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                .arrayOfPointers = false,
                .data = {
                    .deviceAddress = instanceDataDeviceAddress,
                },
            }},
    };

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = 1, // Has to be exactly one for a TLAS
        .pGeometries = &geometry,
    };

    auto sizes = tlas->getBuildSizes(
        &primitiveCount, &buildGeometryInfo,
        VkPhysicalDeviceAccelerationStructurePropertiesKHR {
            .minAccelerationStructureScratchOffsetAlignment = 128, /* Specific value for my 2080 */
        });

    if (tlas->scratchBuffer == nullptr || sizes.buildScratchSize > tlas->scratchBuffer->getSize()) {
        if (tlas->scratchBuffer != nullptr)
            tlas->scratchBuffer->destroy();
        tlas->createScratchBuffer(sizes);
    }

    if (tlas->resultBuffer == nullptr || sizes.accelerationStructureSize > tlas->resultBuffer->getSize()) {
        if (tlas->resultBuffer != nullptr)
            tlas->resultBuffer->destroy();
        tlas->createResultBuffer(sizes);
        tlas->createStructure(sizes);

        /* We created a new TLAS handle, we have to re-bind its descriptor as well. */
        auto tlasDescriptorWrite = tlas->getDescriptorWrite();
        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = &tlasDescriptorWrite,
            .dstSet = pipeline->descriptorSet,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
        };
        vkUpdateDescriptorSets(*device, 1, &descriptorWrite, 0, nullptr);
    }

    device->setDebugUtilsName(tlas->handle, "TLAS");

    buildGeometryInfo.dstAccelerationStructure = tlas->handle;
    buildGeometryInfo.scratchData.deviceAddress = tlas->scratchBuffer->getDeviceAddress();

    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo = {
        .primitiveCount = primitiveCount,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0,
    };
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> buildRangeInfos = {&buildRangeInfo};
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos = {buildGeometryInfo};

    tlasInstanceStagingBuffer->copyToBuffer(cmdBuffer, tlasInstanceBuffer.get());
    VkMemoryBarrier memBarrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
    };
    cmdBuffer->pipelineBarrier(
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0,
        1, &memBarrier, 0, nullptr, 0, nullptr);
    cmdBuffer->setCheckpoint("Building TLAS!");
    cmdBuffer->buildAccelerationStructures(buildGeometryInfos, buildRangeInfos);
}

krypton::rapi::RenderObjectHandle krypton::rapi::VulkanRT_RAPI::createRenderObject() {
    return static_cast<krypton::rapi::RenderObjectHandle>(objects.getNewHandle());
}

bool krypton::rapi::VulkanRT_RAPI::destroyRenderObject(krypton::rapi::RenderObjectHandle& handle) {
    auto valid = objects.isHandleValid(handle);
    if (valid) {
        /** Also delete the buffers */
        auto& object = objects.getFromHandle(handle);
        object.blas->destroyMeshBuffers();
        object.blas->destroy();

        objects.removeHandle(handle);
    }
    return valid;
}

void krypton::rapi::VulkanRT_RAPI::drawFrame() {
    if (needsResize)
        return;

    /* We'll firstly update the camera buffer */
    cameraBuffer->memoryCopy(cameraData.get(), krypton::rapi::CAMERA_DATA_SIZE);

    auto image = swapchain->swapchainImages[static_cast<size_t>(swapchainIndex)];
    drawCommandBuffer->begin(0);

    this->buildTLAS(drawCommandBuffer.get());

    if (handlesForFrame.empty()) {
        /* We won't render anything this frame anyway. Just transition the 
         * swapchain image and end the command buffer. */
        carbon::Image::changeLayout(image, drawCommandBuffer.get(),
                                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

        drawCommandBuffer->end(graphicsQueue.get());
        return;
    }

    vkCmdBindPipeline(*drawCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline->pipeline);
    vkCmdBindDescriptorSets(*drawCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline->pipelineLayout, 0, 1, &pipeline->descriptorSet, 0, nullptr);

    VkStridedDeviceAddressRegionKHR callableRegion = {};
    drawCommandBuffer->setCheckpoint("Tracing rays.");
    drawCommandBuffer->traceRays(
        &rayGenShader.region,
        &missShader.region,
        &closestHitShader.region,
        &callableRegion,
        storageImage->getImageSize3d());

    drawCommandBuffer->setCheckpoint("Changing image layout.");
    storageImage->changeLayout(
        drawCommandBuffer.get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT);

    carbon::Image::changeLayout(image, drawCommandBuffer.get(),
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    drawCommandBuffer->setCheckpoint("Copying storage image.");
    storageImage->copyImage(drawCommandBuffer.get(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    storageImage->changeLayout(
        drawCommandBuffer.get(), VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    carbon::Image::changeLayout(image, drawCommandBuffer.get(),
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                // Fragment shader here, as the next stage will be imgui drawing.
                                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    drawCommandBuffer->end(graphicsQueue.get());
}

void krypton::rapi::VulkanRT_RAPI::endFrame() {
    auto result = submitFrame();
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        needsResize = true;
    } else {
        checkResult(graphicsQueue.get(), result, "Failed to submit queue!");
    }

    handlesForFrame.clear();
}

std::shared_ptr<krypton::rapi::Window> krypton::rapi::VulkanRT_RAPI::getWindow() {
    return window;
}

void krypton::rapi::VulkanRT_RAPI::init() {
    window->create(krypton::rapi::Backend::Vulkan);

    int width, height;
    window->getWindowSize(&width, &height);
    frameBufferWidth = width;
    frameBufferHeight = height;

    auto exts = window->getVulkanExtensions();
    for (auto& e : exts) {
        std::string ext = std::string {e};
        instanceExtensions.push_back(ext);
        fmt::print("Requesting window extension: {}\n", ext);
    }
    instance->addExtensions(instanceExtensions);
    instance->create();
    surface = window->createVulkanSurface(VkInstance(*instance));

    // Create device and allocator
    physicalDevice->create(instance, surface);
    device->create(physicalDevice);

    fmt::print("Setting up Vulkan on: {}\n", physicalDevice->getDeviceName());

    // Create our VMA Allocator
    VmaAllocatorCreateInfo allocatorInfo = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = *physicalDevice,
        .device = *device,
        .instance = *instance};
    vmaCreateAllocator(&allocatorInfo, &allocator);

    // Create our render sync structures
    renderFence->create(VK_FENCE_CREATE_SIGNALED_BIT);
    presentCompleteSemaphore->create();
    renderCompleteSemaphore->create();

    // Create our simple command buffers
    commandPool->create(device->getQueueIndex(vkb::QueueType::graphics), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    drawCommandBuffer = commandPool->allocateBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 0);

    // Create queue and swapchain
    graphicsQueue->create(vkb::QueueType::graphics);
    swapchain->create(surface, {frameBufferWidth, frameBufferHeight});

    pipeline = nullptr;
    shaderBindingTable = std::make_unique<carbon::Buffer>(device, allocator, "shaderBindingTable");
    storageImage = std::make_unique<carbon::StorageImage>(device, allocator, VkExtent2D {frameBufferWidth, frameBufferHeight});

    tlas = std::make_unique<carbon::TopLevelAccelerationStructure>(device, allocator); /* TLAS needs the allocator, which doesn't exist in the ctor */
    tlasInstanceBuffer = std::make_unique<carbon::Buffer>(device, allocator, "tlasInstanceBuffer");
    tlasInstanceStagingBuffer = std::make_unique<carbon::StagingBuffer>(device, allocator, "tlasInstanceStagingBuffer");

    cameraBuffer = std::make_unique<carbon::Buffer>(device, allocator, "cameraBuffer");
    cameraBuffer->create(krypton::rapi::CAMERA_DATA_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    cameraBuffer->memoryCopy(&cameraData, krypton::rapi::CAMERA_DATA_SIZE);

    // Create shaders
    rayGenShader.shader = std::make_unique<carbon::ShaderModule>(device, "RayGen", carbon::ShaderStage::RayGeneration);
    missShader.shader = std::make_unique<carbon::ShaderModule>(device, "RayMiss", carbon::ShaderStage::RayMiss);
    closestHitShader.shader = std::make_unique<carbon::ShaderModule>(device, "ClosestHit", carbon::ShaderStage::ClosestHit);
    anyHitShader.shader = std::make_unique<carbon::ShaderModule>(device, "AnyHit", carbon::ShaderStage::AnyHit);

    rayGenShader.shader->createShader("shaders/raygen.rgen");
    missShader.shader->createShader("shaders/miss.rmiss");
    closestHitShader.shader->createShader("shaders/closesthit.rchit");
    anyHitShader.shader->createShader("shaders/anyhit.rahit");

    window->setRapiPointer(this);

    /* Build necessary objects for rendering */
    storageImage->create();
    oneTimeSubmit(graphicsQueue.get(), commandPool.get(), [&](carbon::CommandBuffer* cmdBuffer) {
        storageImage->changeLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    });

    this->buildPipeline();
    this->buildSBT();
}

void krypton::rapi::VulkanRT_RAPI::loadMeshForRenderObject(
    krypton::rapi::RenderObjectHandle& handle, std::shared_ptr<krypton::mesh::Mesh> mesh) {
    if (objects.isHandleValid(handle)) {
        auto& renderObject = objects.getFromHandle(handle);

        /**
         * The render object should not have been created yet, so
         * all of it's members should still be nullptrs.
         */
        renderObject.mesh = std::move(mesh);
        renderObject.blas = std::make_unique<carbon::BottomLevelAccelerationStructure>(device, allocator, renderObject.mesh->name);

        /** We'll build the BLAS here too */
        buildBLAS(renderObject);
    }
}

void krypton::rapi::VulkanRT_RAPI::oneTimeSubmit(carbon::Queue* queue, carbon::CommandPool* pool,
                                                 const std::function<void(carbon::CommandBuffer*)>& callback) const {
    auto cmdBuffer = pool->allocateBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    cmdBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    callback(cmdBuffer.get());
    cmdBuffer->end(queue);

    VkCommandBuffer vkCmdBuffer = VkCommandBuffer(*cmdBuffer);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkCmdBuffer,
    };
    carbon::Fence fence(device, "oneTimeSubmitFence");
    fence.create(0);
    auto res = queue->submit(&fence, &submitInfo);
    checkResult(queue, res, "Failed to submit queue");

    fence.wait();
    fence.destroy();
}

void krypton::rapi::VulkanRT_RAPI::render(RenderObjectHandle& handle) {
    handlesForFrame.emplace_back(handle);
}

void krypton::rapi::VulkanRT_RAPI::resize(int width, int height) {
    auto result = device->waitIdle();
    checkResult(graphicsQueue.get(), result, "Failed to wait on device idle");

    frameBufferWidth = width;
    frameBufferHeight = height;

    // Re-create the swapchain.
    swapchain->create(surface, {frameBufferWidth, frameBufferHeight});

    /* Recreate the storage image */
    storageImage->recreateImage({frameBufferWidth, frameBufferHeight});
    oneTimeSubmit(graphicsQueue.get(), commandPool.get(), [&](carbon::CommandBuffer* cmdBuffer) {
        storageImage->changeLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    });

    /* Re-bind the storage image */
    VkDescriptorImageInfo storageImageDescriptor = storageImage->getDescriptorImageInfo();
    VkWriteDescriptorSet resultImageWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = pipeline->descriptorSet,
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &storageImageDescriptor,
    };
    vkUpdateDescriptorSets(*device, 1, &resultImageWrite, 0, nullptr);

    needsResize = false;
}

void krypton::rapi::VulkanRT_RAPI::shutdown() {
    checkResult(graphicsQueue.get(), device->waitIdle(),
                "Failed waiting on device idle");

    pipeline->destroy(device.get());

    cameraBuffer->destroy();
    shaderBindingTable->destroy();

    tlasInstanceStagingBuffer->destroy();
    tlasInstanceBuffer->destroy();
    tlas->destroy();

    storageImage->destroy();
    swapchain->destroy();

    presentCompleteSemaphore->destroy();
    renderCompleteSemaphore->destroy();
    renderFence->destroy();

    vkDestroySurfaceKHR(*instance, surface, nullptr);

    commandPool->destroy();

    vmaDestroyAllocator(allocator);

    device->destroy();
    instance->destroy();
    window->destroy();
}

void krypton::rapi::VulkanRT_RAPI::setCameraData(std::shared_ptr<krypton::rapi::CameraData> cameraData) {
    this->cameraData = std::move(cameraData);
}

VkResult krypton::rapi::VulkanRT_RAPI::submitFrame() {
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};

    VkCommandBuffer cmdBuffer = VkCommandBuffer(*drawCommandBuffer);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &presentCompleteSemaphore->getHandle(),
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderCompleteSemaphore->getHandle(),
    };
    auto result = graphicsQueue->submit(renderFence.get(), &submitInfo);
    if (result != VK_SUCCESS) {
        checkResult(graphicsQueue.get(), result, "Failed to submit queue");
    }

    return swapchain->queuePresent(graphicsQueue, swapchainIndex, renderCompleteSemaphore);
}

VkResult krypton::rapi::VulkanRT_RAPI::waitForFrame() {
    renderFence->wait();
    renderFence->reset();
    return swapchain->acquireNextImage(presentCompleteSemaphore, &swapchainIndex);
}

#endif // #ifdef RAPI_WITH_VULKAN
