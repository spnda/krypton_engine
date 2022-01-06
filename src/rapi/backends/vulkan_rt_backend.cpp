#ifdef RAPI_WITH_VULKAN

#include <algorithm> 
#include <cctype>
#include <locale>
#include <iostream>

#include <fmt/core.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

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
#include <carbon/rt/acceleration_structure.hpp>
#include <carbon/utils.hpp>

#include <rapi/backends/vulkan_rt_backend.hpp>
#include <rapi/render_object_handle.hpp>

krypton::rapi::VulkanRT_RAPI::VulkanRT_RAPI() {
    instance = std::make_shared<carbon::Instance>();
    instance->setApplicationData({
        .apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0),
        .applicationVersion = 1,
        .engineVersion = 1,

        .applicationName = "Krypton",
        .engineName = "Krypton Engine"
    });

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

        std::span<std::byte> vertex = { reinterpret_cast<std::byte*>(prim.vertices.data()), prim.vertices.size() * krypton::mesh::VERTEX_STRIDE };
        std::span<std::byte> index = { reinterpret_cast<std::byte*>(prim.indices.data()), prim.indices.size() * sizeof(uint32_t) };

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
                        .deviceAddress = renderObject.blas->vertexBuffer.getDeviceAddress()
                            + renderObject.geometryDescriptions[primitiveIndex].meshBufferVertexOffset,
                    },
                    .vertexStride = krypton::mesh::VERTEX_STRIDE,
                    .maxVertex = static_cast<uint32_t>(prim.vertices.size() - 1),
                    .indexType = VK_INDEX_TYPE_UINT32,
                    .indexData = {
                        .deviceAddress = renderObject.blas->indexBuffer.getDeviceAddress()
                            + renderObject.geometryDescriptions[primitiveIndex].meshBufferIndexOffset,
                    },
                    .transformData = {
                        .deviceAddress = renderObject.blas->transformBuffer.getDeviceAddress(),
                    },
                },
            }
        };

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
        }
    );
    renderObject.blas->createScratchBuffer(sizes);
    renderObject.blas->createResultBuffer(sizes);
    renderObject.blas->createStructure(sizes);
    device->setDebugUtilsName(renderObject.blas->handle, renderObject.mesh->name);

    buildGeometryInfo.dstAccelerationStructure = renderObject.blas->handle;
    buildGeometryInfo.scratchData.deviceAddress = renderObject.blas->scratchBuffer->getDeviceAddress();

    /** We're only building one BLAS here currently. */
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos = { buildGeometryInfo };
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

void krypton::rapi::VulkanRT_RAPI::buildTLAS(carbon::CommandBuffer* cmdBuffer) {
    const uint32_t primitiveCount = handlesForFrame.size();

    std::vector<VkAccelerationStructureInstanceKHR> instances(primitiveCount, VkAccelerationStructureInstanceKHR {});
    for (auto& handle : handlesForFrame) {
        size_t index = &handle - &handlesForFrame[0];
        instances[index].transform = {
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
        };
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
            }
        },
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
        }
    );

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
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> buildRangeInfos = { &buildRangeInfo };
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos = { buildGeometryInfo };

    tlasInstanceStagingBuffer->copyToBuffer(cmdBuffer, *tlasInstanceBuffer);
    VkMemoryBarrier memBarrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
    };
    cmdBuffer->pipelineBarrier(
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0,
        1, &memBarrier, 0, nullptr, 0, nullptr
    );
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

    auto image = swapchain->swapchainImages[static_cast<size_t>(swapchainIndex)];
    drawCommandBuffer->begin(0);

    this->buildTLAS(drawCommandBuffer.get());

    carbon::Image::changeLayout(image, drawCommandBuffer,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

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
        std::string ext = std::string { e };
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
        .instance = *instance
    };
    vmaCreateAllocator(&allocatorInfo, &allocator);

    // Create our render sync structures
    renderFence->create(VK_FENCE_CREATE_SIGNALED_BIT);
    presentCompleteSemaphore->create();
    renderCompleteSemaphore->create();

    // Create our simple command buffers
    commandPool->create(device->getQueueIndex(vkb::QueueType::graphics), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    auto cmdBuffers = commandPool->allocateBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 0, 1);
    drawCommandBuffer = cmdBuffers.front();

    // Create queue and swapchain
    graphicsQueue->create(vkb::QueueType::graphics);
    swapchain->create(surface, { frameBufferWidth, frameBufferHeight });

    tlas = std::make_unique<carbon::TopLevelAccelerationStructure>(device, allocator); /* TLAS needs the allocator, which doesn't exist in the ctor */
    tlasInstanceBuffer = std::make_unique<carbon::Buffer>(device, allocator, "tlasInstanceBuffer");
    tlasInstanceStagingBuffer = std::make_unique<carbon::StagingBuffer>(device, allocator, "tlasInstanceStagingBuffer");

    window->setRapiPointer(this);
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
    swapchain->create(surface, { frameBufferWidth, frameBufferHeight });
    
    needsResize = false;
}

void krypton::rapi::VulkanRT_RAPI::shutdown() {
    checkResult(graphicsQueue.get(), device->waitIdle(),
        "Failed waiting on device idle");

    tlasInstanceStagingBuffer->destroy();
    tlasInstanceBuffer->destroy();
    tlas->destroy();

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

VkResult krypton::rapi::VulkanRT_RAPI::submitFrame() {
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };

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
