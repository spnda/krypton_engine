#ifdef RAPI_WITH_VULKAN

#include <array>
#include <cctype>

#include <imgui.h>

#define VMA_IMPLEMENTATION
#define VMA_ASSERT(expr) // We don't want VMA to do any assertions.

#include <carbon/base/command_buffer.hpp>
#include <carbon/base/command_pool.hpp>
#include <carbon/base/device.hpp>
#include <carbon/base/fence.hpp>
#include <carbon/base/instance.hpp>
#include <carbon/base/physical_device.hpp>
#include <carbon/base/queue.hpp>
#include <carbon/base/semaphore.hpp>
#include <carbon/base/swapchain.hpp>
#include <carbon/pipeline/descriptor_set.hpp>
#include <carbon/pipeline/graphics_pipeline.hpp>
#include <carbon/resource/buffer.hpp>
#include <carbon/resource/image.hpp>
#include <carbon/resource/stagingbuffer.hpp>
#include <carbon/resource/storageimage.hpp>
#include <carbon/resource/texture.hpp>
#include <carbon/rt/acceleration_structure.hpp>
#include <carbon/rt/rt_pipeline.hpp>
#include <carbon/shaders/shader.hpp>
#include <carbon/utils.hpp>
#include <carbon/vulkan.hpp>
#include <rapi/backends/vulkan_backend.hpp>
#include <shaders/shaders.hpp>
#include <threading/scheduler.hpp>
#include <util/logging.hpp>

static std::map<carbon::ShaderStage, krypton::shaders::ShaderStage> carbonShaderKinds = {
    { carbon::ShaderStage::RayGeneration, krypton::shaders::ShaderStage::RayGen },
    { carbon::ShaderStage::ClosestHit, krypton::shaders::ShaderStage::ClosestHit },
    { carbon::ShaderStage::RayMiss, krypton::shaders::ShaderStage::Miss },
    { carbon::ShaderStage::AnyHit, krypton::shaders::ShaderStage::AnyHit },
    { carbon::ShaderStage::Intersection, krypton::shaders::ShaderStage::Intersect },
    { carbon::ShaderStage::Callable, krypton::shaders::ShaderStage::Callable },
};

struct ImGuiPushConstants {
    glm::fvec2 scale;
    glm::fvec2 translate;
};

namespace krypton::rapi::vulkan {
    VkTransformMatrixKHR glmToVulkanMatrix(const glm::mat4x3& glmMatrix) {
        VkTransformMatrixKHR vkMatrix;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 4; ++j)
                vkMatrix.matrix[i][j] = glmMatrix[j][i];
        return vkMatrix;
    }

    inline VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                              VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                krypton::log::err(pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                krypton::log::warn(pCallbackData->pMessage);
                break;
            default:
                krypton::log::log(pCallbackData->pMessage);
                break;
        }

        return VK_FALSE;
    }
} // namespace krypton::rapi::vulkan

krypton::rapi::VulkanBackend::VulkanBackend() {
    instance = std::make_unique<carbon::Instance>();
    instance->setDebugCallback(krypton::rapi::vulkan::vulkanDebugCallback);
    instance->setApplicationData({ .apiVersion = VK_API_VERSION_1_3,
                                   .applicationVersion = 1,
                                   .engineVersion = 1,

                                   .applicationName = "Krypton",
                                   .engineName = "Krypton Engine" });

    physicalDevice = std::make_shared<carbon::PhysicalDevice>();
    device = std::make_shared<carbon::Device>();
    window = std::make_shared<krypton::rapi::Window>("Krypton", 1920, 1080);

    renderFence = std::make_shared<carbon::Fence>(device, "renderFence");
    presentCompleteSemaphore = std::make_shared<carbon::Semaphore>(device, "presentComplete");
    renderCompleteSemaphore = std::make_shared<carbon::Semaphore>(device, "renderComplete");

    swapchain = std::make_shared<carbon::Swapchain>(instance.get(), device);
    graphicsQueue = std::make_shared<carbon::Queue>(device, "graphicsQueue");
    commandPool = std::make_shared<carbon::CommandPool>(device, "commandPool");

    blasComputeQueue = std::make_unique<carbon::Queue>(device, "blasComputeQueue");
    computeCommandPool = std::make_shared<carbon::CommandPool>(device, "computeCommandPool");

    // Create camera buffer
    cameraData = std::make_shared<krypton::rapi::CameraData>();
    convertedCameraData = std::make_unique<krypton::rapi::CameraData>();
}

krypton::rapi::VulkanBackend::~VulkanBackend() = default;

void krypton::rapi::VulkanBackend::addPrimitive(krypton::util::Handle<"RenderObject">& handle, krypton::assets::Primitive& primitive,
                                                krypton::util::Handle<"Material">& material) {
    auto lock = std::scoped_lock(renderObjectMutex);
    auto& object = objects.getFromHandle(handle);
    object.primitives.push_back({ primitive, material });
}

void krypton::rapi::VulkanBackend::beginFrame() {
    if (!needsResize) {
        window->pollEvents();

        auto result = waitForFrame();
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            needsResize = true;
        } else {
            checkResult(graphicsQueue.get(), result, "Failed to present queue!");
        }
    } else {
        window->waitEvents(); // We'd usually return but we want imgui to still run
    }

    // We'll also update ImGui here. This allows
    // the caller to draw any UI between beginFrame
    // and drawFrame.
    window->newFrame();
    ImGui::NewFrame();
}

void krypton::rapi::VulkanBackend::buildRenderObject(krypton::util::Handle<"RenderObject">& handle) {
    auto lock = std::unique_lock(renderObjectMutex);
    auto& renderObject = objects.getFromHandle(handle);

    auto primitiveSize = renderObject.primitives.size();
    std::vector<uint32_t> primitiveCounts(primitiveSize);
    std::vector<VkAccelerationStructureGeometryKHR> geometries(primitiveSize);
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfos(primitiveSize);

    renderObject.geometryDescriptions.reserve(primitiveSize);
    renderObject.blas = std::make_unique<carbon::BottomLevelAccelerationStructure>(device, allocator, "");

    /** Calculate the total buffer sizes and offsets */
    std::vector<carbon::BottomLevelAccelerationStructure::PrimitiveData> primitiveData(primitiveSize);
    uint64_t totalVertexSize = 0, totalIndexSize = 0;
    for (auto& primitive : renderObject.primitives) {
        auto& description = renderObject.geometryDescriptions.emplace_back();
        description.materialIndex = static_cast<krypton::assets::Index>(primitive.material.getIndex());

        // We use the address beforehand to store an offset into the actual address.
        description.vertexBufferAddress = totalVertexSize;
        description.indexBufferAddress = totalIndexSize;

        auto& prim = primitive.primitive;
        std::span<std::byte> vertex = { reinterpret_cast<std::byte*>(prim.vertices.data()),
                                        prim.vertices.size() * krypton::assets::VERTEX_STRIDE };
        std::span<std::byte> index = { reinterpret_cast<std::byte*>(prim.indices.data()), prim.indices.size() * sizeof(uint32_t) };

        totalVertexSize += vertex.size();
        totalIndexSize += index.size();

        primitiveData.emplace_back(vertex, index);
    }

    lock.unlock();

    VkTransformMatrixKHR blasTransform = vulkan::glmToVulkanMatrix(renderObject.transform);
    renderObject.blas->createMeshBuffers(primitiveData, blasTransform);

    auto vertexDeviceAddress = renderObject.blas->vertexBuffer->getDeviceAddress();
    auto indexDeviceAddress = renderObject.blas->indexBuffer->getDeviceAddress();

    /** Generate the geometry data with the buffer info just generated */
    for (const auto& prim : renderObject.primitives) {
        uint64_t primitiveIndex = &prim - &renderObject.primitives.front();
        // maxPrimitiveCount may not be larger than UINT32_MAX
        primitiveCounts[primitiveIndex] = std::min((uint32_t)prim.primitive.indices.size() / 3, (uint32_t)asProperties->maxPrimitiveCount);

        // We use the address beforehand to store an offset into the actual address.
        auto vertexAddress = vertexDeviceAddress + renderObject.geometryDescriptions[primitiveIndex].vertexBufferAddress;
        auto indexAddress = indexDeviceAddress + renderObject.geometryDescriptions[primitiveIndex].indexBufferAddress;

        renderObject.geometryDescriptions[primitiveIndex].vertexBufferAddress = vertexAddress;
        renderObject.geometryDescriptions[primitiveIndex].indexBufferAddress = indexAddress;

        geometries[primitiveIndex] = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            .geometry = {
                .triangles = {
                    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                    .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT, // TODO: Replace with variable
                    .vertexData {
                        .deviceAddress = vertexAddress,
                    },
                    .vertexStride = krypton::assets::VERTEX_STRIDE,
                    .maxVertex = static_cast<uint32_t>(prim.primitive.vertices.size() - 1),
                    .indexType = VK_INDEX_TYPE_UINT32,
                    .indexData = {
                        .deviceAddress = indexAddress,
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
    auto sizes = renderObject.blas->getBuildSizes(primitiveCounts.data(), &buildGeometryInfo, *asProperties);
    renderObject.blas->createScratchBuffer(sizes);
    renderObject.blas->createResultBuffer(sizes);
    renderObject.blas->createStructure(sizes);
    device->setDebugUtilsName(renderObject.blas->handle, "");

    buildGeometryInfo.dstAccelerationStructure = renderObject.blas->handle;
    buildGeometryInfo.scratchData.deviceAddress = renderObject.blas->scratchBuffer->getDeviceAddress();

    /** We're only building one BLAS here currently. */
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos = { buildGeometryInfo };
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> rangeInfoPtrs;
    rangeInfoPtrs.push_back(rangeInfos.data());

    /** Dispatch the commands */
    oneTimeSubmit(blasComputeQueue.get(), computeCommandPool.get(), [&](carbon::CommandBuffer* cmdBuffer) {
        /* First copy our mesh buffers */
        cmdBuffer->setCheckpoint("Copying mesh buffers!");
        renderObject.blas->copyMeshBuffers(cmdBuffer);

        /* Insert memory barrier for the buffer copies */
        VkMemoryBarrier memBarrier = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
        };
        cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1,
                                   &memBarrier, 0, nullptr, 0, nullptr);

        /* Build the AS */
        cmdBuffer->setCheckpoint("Building BLAS!");
        cmdBuffer->buildAccelerationStructures(buildGeometryInfos, rangeInfoPtrs);
    });

    renderObject.blas->destroyMeshBuffers();
}

void krypton::rapi::VulkanBackend::buildRTPipeline() {
    rtProperties = std::make_unique<VkPhysicalDeviceRayTracingPipelinePropertiesKHR>();
    rtProperties->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    physicalDevice->getProperties(rtProperties.get());

    if (!rayGenShader.shader || !missShader.shader || !closestHitShader.shader || !anyHitShader.shader) {
        // No shaders, can't build pipeline.
        return;
    }

    if (rtPipeline != nullptr)
        rtPipeline->destroy();

    rtDescriptorSet = std::make_shared<carbon::DescriptorSet>(device.get());

    /* Add descriptors */
    rtDescriptorSet->addBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, carbon::ShaderStage::RayGeneration);

    rtDescriptorSet->addBuffer(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                               carbon::ShaderStage::ClosestHit |
                                   carbon::ShaderStage::AnyHit); // VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);

    rtDescriptorSet->addBuffer(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, carbon::ShaderStage::ClosestHit);

    rtDescriptorSet->addAccelerationStructure(3, carbon::ShaderStage::RayGeneration | carbon::ShaderStage::ClosestHit);

    rtDescriptorSet->addImage(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, carbon::ShaderStage::RayGeneration);

    rtDescriptorSet->create();

    rtPipeline = std::make_unique<carbon::RayTracingPipeline>(device.get());
    rtPipeline->addDescriptorSet(rtDescriptorSet);
    rtPipeline->addShaderGroup(carbon::RtShaderGroup::General, { rayGenShader.shader.get() });
    rtPipeline->addShaderGroup(carbon::RtShaderGroup::General, { missShader.shader.get() });
    rtPipeline->addShaderGroup(carbon::RtShaderGroup::TriangleHit, { closestHitShader.shader.get(), anyHitShader.shader.get() });
    rtPipeline->create();
    rtPipeline->setName("rt_pipeline");

    auto cameraBufferInfo = cameraBuffer->getDescriptorInfo(VK_WHOLE_SIZE, 0);
    auto storageImageDescriptor = storageImage->getDescriptorImageInfo();
    rtDescriptorSet->updateBuffer(0, &cameraBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    rtDescriptorSet->updateImage(4, &storageImageDescriptor, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
}

void krypton::rapi::VulkanBackend::buildSBT() {
    // The pipeline wasn't built or is invalid.
    if (!rtPipeline)
        return;

    uint32_t missCount = 1; // 1 miss groups
    uint32_t chitCount = 1; // 1 hit group (with closest and any)
    uint32_t callCount = 0;
    auto handleCount = 1 + missCount + chitCount + callCount;
    const uint32_t handleSize = rtProperties->shaderGroupHandleSize;
    sbtStride = carbon::Buffer::alignedSize(handleSize, rtProperties->shaderGroupHandleAlignment);

    rayGenShader.region.stride = carbon::Buffer::alignedSize(sbtStride, rtProperties->shaderGroupBaseAlignment);
    rayGenShader.region.size = rayGenShader.region.stride; // RayGen size must be equal to the stride.
    missShader.region.stride = sbtStride;
    missShader.region.size = carbon::Buffer::alignedSize(missCount * sbtStride, rtProperties->shaderGroupBaseAlignment);
    closestHitShader.region.stride = sbtStride;
    closestHitShader.region.size = carbon::Buffer::alignedSize(chitCount * sbtStride, rtProperties->shaderGroupBaseAlignment);

    const size_t sbtSize = rayGenShader.region.size + missShader.region.size + closestHitShader.region.size;
    std::vector<uint8_t> handleStorage(sbtSize);
    auto result = rtPipeline->getShaderGroupHandles(handleCount, handleStorage);
    checkResult(result, "Failed to get ray tracing shader group handles!");

    shaderBindingTable->create(sbtSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                   VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                               VMA_MEMORY_USAGE_CPU_TO_GPU);
    auto sbtAddress = shaderBindingTable->getDeviceAddress();
    rayGenShader.region.deviceAddress = sbtAddress;
    missShader.region.deviceAddress = sbtAddress + rayGenShader.region.size;
    closestHitShader.region.deviceAddress = missShader.region.deviceAddress + missShader.region.size;

    auto getHandleOffset = [&](uint32_t i) -> auto {
        return handleStorage.data() + i * handleSize;
    };
    uint32_t curHandleIndex = 0;

    // Write raygen shader
    shaderBindingTable->memoryCopy(getHandleOffset(curHandleIndex++), handleSize, 0);

    // Write miss shaders
    for (uint32_t i = 0; i < missCount; i++) {
        shaderBindingTable->memoryCopy(getHandleOffset(curHandleIndex++), handleSize,
                                       rayGenShader.region.size + missShader.region.stride * i);
    }

    // Write chit shaders
    for (uint32_t i = 0; i < chitCount; i++) {
        shaderBindingTable->memoryCopy(getHandleOffset(curHandleIndex++), handleSize,
                                       rayGenShader.region.size + missShader.region.size + closestHitShader.region.stride * i);
    }
}

void krypton::rapi::VulkanBackend::buildUIPipeline() {
    if (!uiFragment || !uiVertex) {
        // No shaders, can't build pipeline.
        return;
    }

    uiDescriptorSet = std::make_shared<carbon::DescriptorSet>(device.get());

    // Add font texture for first descriptor binding.
    uiDescriptorSet->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, carbon::ShaderStage::Fragment);

    uiDescriptorSet->create();

    uiPipeline = std::make_unique<carbon::GraphicsPipeline>(device.get());
    uiPipeline->addDescriptorSet(uiDescriptorSet);
    uiPipeline->addShaderModule(uiFragment.get());
    uiPipeline->addShaderModule(uiVertex.get());
    uiPipeline->addPushConstant(sizeof(ImGuiPushConstants), carbon::ShaderStage::Vertex);

    // clang-format off
    auto swapchainAttachment = uiPipeline->addColorAttachment(swapchain->getFormat());
    uiPipeline->setBlendingForColorAttachment(swapchainAttachment, {
        .blendEnable = true,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM,
    });
    // clang-format on

    uint32_t vBinding = uiPipeline->addVertexBinding({
        .binding = 0,
        .stride = sizeof(ImDrawVert),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    });

    // Position
    uiPipeline->addVertexAttribute({
        .location = 0,
        .binding = vBinding,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = static_cast<uint32_t>(offsetof(ImDrawVert, pos)),
    });

    // UV
    uiPipeline->addVertexAttribute({
        .location = 1,
        .binding = vBinding,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = static_cast<uint32_t>(offsetof(ImDrawVert, uv)),
    });

    // Color
    uiPipeline->addVertexAttribute({
        .location = 2,
        .binding = vBinding,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .offset = static_cast<uint32_t>(offsetof(ImDrawVert, col)),
    });

    uiPipeline->create();
    uiPipeline->setName("ui_pipeline");

    auto fontTextureInfo = uiFontTexture->getDescriptorImageInfo();
    uiDescriptorSet->updateImage(0, &fontTextureInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
}

void krypton::rapi::VulkanBackend::buildTLAS(carbon::CommandBuffer* cmdBuffer) {
    frameHandleMutex.lock();
    renderObjectMutex.lock();

    // As per the Vulkan spec, no more than UINT32_MAX primitives/instances are
    // allowed inside of a single TLAS. This represents the maximum amount of instances
    // this TLAS can have, including possibly invalid handles.
    const auto maxPrimitiveCount = static_cast<uint32_t>(handlesForFrame.size());

    // We keep track of the actual instance count as some handles might be invalid
    // or not ready to be used yet.
    uint32_t primitiveCount = 0;
    std::vector<VkAccelerationStructureInstanceKHR> instances(maxPrimitiveCount, VkAccelerationStructureInstanceKHR {});
    std::vector<krypton::rapi::vulkan::GeometryDescription> descriptions = {};
    for (auto& handle : handlesForFrame) {
        if (!objects.isHandleValid(handle))
            continue; // The handle is invalid; the object was probably destroyed

        auto& object = objects.getFromHandle(handle);
        if (!object.blas)
            continue; /* The BLAS is invalid, currently being rebuilt, or not yet built */

        size_t index = &handle - &handlesForFrame[0];
        // clang-format off
        instances[index].transform = {
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
        };
        // clang-format on
        instances[index].instanceCustomIndex = static_cast<uint32_t>(descriptions.size());
        instances[index].mask = 0xFF;
        instances[index].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instances[index].instanceShaderBindingTableRecordOffset = 0;
        instances[index].accelerationStructureReference = object.blas->address;

        // We increase the size of the descriptions vector and insert
        descriptions.insert(descriptions.end(), object.geometryDescriptions.begin(), object.geometryDescriptions.end());

        ++primitiveCount;
    }

    renderObjectMutex.unlock();
    frameHandleMutex.unlock();

    VkDeviceAddress instanceDataDeviceAddress = {};

    if (primitiveCount > 0) {
        size_t instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * primitiveCount;
        size_t descriptionBufferSize = sizeof(krypton::rapi::vulkan::GeometryDescription) * descriptions.size();

        if (instanceBufferSize <= tlasInstanceBuffer->getSize() && tlasInstanceBuffer->getSize() != 0) {
            // We can re-use the old instance buffers
            tlasInstanceBuffer->memoryCopy(instances.data(), instanceBufferSize);
            instanceDataDeviceAddress = tlasInstanceBuffer->getDeviceAddress(); /* Same address as before */

            geometryDescriptionBuffer->memoryCopy(descriptions.data(), descriptionBufferSize);
        } else {
            // As we cannot use the old buffers, we'll destroy those now
            tlasInstanceBuffer->destroy();
            geometryDescriptionBuffer->destroy();

            tlasInstanceBuffer->create(instanceBufferSize);
            tlasInstanceBuffer->memoryCopy(instances.data(), instanceBufferSize);

            tlasInstanceBuffer->createDestinationBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
            instanceDataDeviceAddress = tlasInstanceBuffer->getDeviceAddress();

            // We can safely assume that if the instance buffer size has increased that the descriptions
            // also have to be reallocated.
            geometryDescriptionBuffer->create(descriptionBufferSize);
            geometryDescriptionBuffer->memoryCopy(descriptions.data(), descriptionBufferSize);

            geometryDescriptionBuffer->createDestinationBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

            auto descriptionBufferInfo = geometryDescriptionBuffer->getDescriptorInfo(VK_WHOLE_SIZE, 0);
            rtDescriptorSet->updateBuffer(1, &descriptionBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
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

    auto sizes = tlas->getBuildSizes(&primitiveCount, &buildGeometryInfo, *asProperties);

    if (tlas->scratchBuffer == nullptr || sizes.buildScratchSize > tlas->scratchBuffer->getSize()) {
        if (tlas->scratchBuffer != nullptr)
            tlas->scratchBuffer->destroy();
        tlas->createScratchBuffer(sizes);
    }

    if (tlas->resultBuffer == nullptr || sizes.accelerationStructureSize > tlas->resultBuffer->getSize()) {
        if (tlas->resultBuffer != nullptr)
            tlas->resultBuffer->destroy();
        tlas->createResultBuffer(sizes);
        tlas->destroyStructure();
        tlas->createStructure(sizes);

        /* We created a new TLAS handle, we have to re-bind its descriptor as well. */
        auto tlasDescriptorWrite = tlas->getDescriptorWrite();
        rtDescriptorSet->updateAccelerationStructure(3, &tlasDescriptorWrite);
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

    tlasInstanceBuffer->copyIntoVram(cmdBuffer);
    geometryDescriptionBuffer->copyIntoVram(cmdBuffer);
    VkMemoryBarrier memBarrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
    };
    cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &memBarrier, 0,
                               nullptr, 0, nullptr);
    cmdBuffer->setCheckpoint("Building TLAS!");
    cmdBuffer->buildAccelerationStructures(buildGeometryInfos, buildRangeInfos);
}

krypton::util::Handle<"RenderObject"> krypton::rapi::VulkanBackend::createRenderObject() {
    auto mutex = std::scoped_lock<std::mutex>(renderObjectMutex);
    auto refCounter = std::make_shared<krypton::util::ReferenceCounter>();
    return objects.getNewHandle(refCounter);
}

krypton::util::Handle<"Material"> krypton::rapi::VulkanBackend::createMaterial(krypton::assets::Material material) {
    auto mutex = std::scoped_lock(materialMutex);
    auto refCounter = std::make_shared<krypton::util::ReferenceCounter>();
    auto handle = materials.getNewHandle(refCounter);
    materials.getFromHandle(handle) = material;
    return std::move(handle);
}

void krypton::rapi::VulkanBackend::createUiFontTexture() {
    uint8_t* pixels;
    int width, height;
    ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
    size_t textureSize = width * height * sizeof(uint8_t); // Single alpha channel

    VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

    // Create the texture
    uiFontTexture = std::make_unique<carbon::Texture>(device, allocator, extent, "uiFontTexture");

    // This essentially set's every value in the texture to vec3(1.0, 1.0, 1.0, val).
    uiFontTexture->setComponentMapping({
        .r = VK_COMPONENT_SWIZZLE_ONE,
        .g = VK_COMPONENT_SWIZZLE_ONE,
        .b = VK_COMPONENT_SWIZZLE_ONE,
        .a = VK_COMPONENT_SWIZZLE_R,
    });
    uiFontTexture->createTexture(VK_FORMAT_R8_UNORM, 1, 1); // This also creates a sampler for us

    // Create the staging buffer for the texture data
    auto textureStagingBuffer = std::make_unique<carbon::StagingBuffer>(device, allocator, "uiFontTextureStagingBuffer");
    textureStagingBuffer->create(textureSize);
    textureStagingBuffer->memoryCopy(pixels, textureSize);

    oneTimeSubmit(graphicsQueue.get(), commandPool.get(), [&](carbon::CommandBuffer* cmdBuffer) {
        auto imageSubresource = VkImageSubresourceRange {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        };
        uiFontTexture->changeLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageSubresource, VK_PIPELINE_STAGE_HOST_BIT,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy copy = {
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
            },
            .imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
        };
        textureStagingBuffer->copyToImage(cmdBuffer, uiFontTexture.get(), uiFontTexture->getImageLayout(), &copy);

        uiFontTexture->changeLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageSubresource, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    });

    textureStagingBuffer->destroy();

    // Store the VkImage handle as a texture ID for ImGui.
    ImGui::GetIO().Fonts->SetTexID((ImTextureID)(intptr_t)VkImage(*uiFontTexture));
}

std::unique_ptr<carbon::ShaderModule> krypton::rapi::VulkanBackend::createShader(const std::string& name, carbon::ShaderStage stage,
                                                                                 krypton::shaders::ShaderCompileResult& result) {
    if (result.resultType == krypton::shaders::CompileResultType::String)
        return nullptr;

    if (result.resultSize <= 0)
        return nullptr;

    auto shader = std::make_unique<carbon::ShaderModule>(device, name, stage);
    shader->createShaderModule((uint32_t*)result.resultBytes.data(), result.resultSize);
    return shader;
}

bool krypton::rapi::VulkanBackend::destroyRenderObject(util::Handle<"RenderObject">& handle) {
    auto lock = std::scoped_lock<std::mutex>(renderObjectMutex);
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

bool krypton::rapi::VulkanBackend::destroyMaterial(util::Handle<"Material">& handle) {
    auto lock = std::scoped_lock(materialMutex);
    auto valid = materials.isHandleValid(handle);
    if (valid) {
        materials.removeHandle(handle);
    }
    return valid;
}

void krypton::rapi::VulkanBackend::drawFrame() {
    if (needsResize)
        return;

    if (frameBufferWidth <= 0 || frameBufferHeight <= 0) {
        // TODO: Find a better way of handling this, instead of
        // just submitting a empty command buffer.
        drawCommandBuffer->begin();
        drawCommandBuffer->end(graphicsQueue.get());
        return;
    }

    // We'll also draw our settings UI now.
    // It is a completely separate window, so it shouldn't interfere
    // with any other imgui calls.
    ImGui::Begin("Renderer Settings");
    ImGui::Text("%s", fmt::format("Rendering on {}", device->getPhysicalDevice()->getDeviceName()).c_str());
    ImGui::Separator();
    for (auto& handle : handlesForFrame) {
        auto& object = objects.getFromHandle(handle);
        ImGui::BulletText("%s", object.name.c_str());
    }
    ImGui::End();

    convertedCameraData->projection = glm::inverse(cameraData->projection);
    convertedCameraData->far = cameraData->far;
    convertedCameraData->view = glm::inverse(cameraData->view);

    // Have to flip the projection matrix because when raytracing, the image is written to the "wrong" way round.
    convertedCameraData->projection[1][1] *= -1.0;
    cameraBuffer->memoryCopy(convertedCameraData.get(), krypton::rapi::CAMERA_DATA_SIZE);

    // Resize the material buffer and update the descriptor.
    materialBuffer->resize(materials.capacity() * sizeof(krypton::assets::Material));
    auto materialBufferInfo = materialBuffer->getDescriptorInfo(VK_WHOLE_SIZE, 0);
    rtDescriptorSet->updateBuffer(2, &materialBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    auto& image = swapchain->swapchainImages[static_cast<size_t>(swapchainIndex)];
    drawCommandBuffer->begin();

    materialBuffer->memoryCopy(materials.data(), materials.size() * sizeof(krypton::assets::Material));
    materialBuffer->copyIntoVram(drawCommandBuffer.get());

    // We update/rebuild the TLAS every frame so that new BLAS instances
    // are added instantly.
    buildTLAS(drawCommandBuffer.get());

    // ↓ ------------------- RT ------------------- ↓
    if (rtPipeline) {
        if (handlesForFrame.empty()) {
            // We won't render anything this frame anyway. Just transition the
            // swapchain image.
            image->changeLayout(drawCommandBuffer.get(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        } else {
            drawCommandBuffer->bindPipeline(rtPipeline.get());
            drawCommandBuffer->bindDescriptorSets(rtPipeline.get());

            // We'll sync the cameraBuffer copy with our ray tracing command.
            // Otherwise our shaders get garbage camera data
            VkBufferMemoryBarrier cameraMemory = cameraBuffer->getMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
            VkBufferMemoryBarrier materialMemory =
                materialBuffer->getMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

            std::array<VkBufferMemoryBarrier, 2> bufferMemoryBarriers = { cameraMemory, materialMemory };
            drawCommandBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 0, nullptr,
                                               2, bufferMemoryBarriers.data(), 0, nullptr);

            VkStridedDeviceAddressRegionKHR callableRegion = {};
            drawCommandBuffer->setCheckpoint("Tracing rays.");
            drawCommandBuffer->traceRays(&rayGenShader.region, &missShader.region, &closestHitShader.region, &callableRegion,
                                         storageImage->getImageSize3d());

            drawCommandBuffer->setCheckpoint("Changing image layout.");
            storageImage->changeLayout(drawCommandBuffer.get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                       VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT);

            image->changeLayout(drawCommandBuffer.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
                                VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT);

            drawCommandBuffer->setCheckpoint("Copying storage image.");
            storageImage->copyImage(drawCommandBuffer.get(), *image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            storageImage->changeLayout(drawCommandBuffer.get(), VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

            image->changeLayout(drawCommandBuffer.get(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        }
    }
    // ↑ ------------------- RT ------------------- ↑

    // ↓ ------------------- UI ------------------- ↓
    if (uiPipeline) {
        drawCommandBuffer->setCheckpoint("Drawing UI");
        drawCommandBuffer->bindPipeline(uiPipeline.get());
        drawCommandBuffer->bindDescriptorSets(uiPipeline.get());

        // We call ImGui::Render ourselves, so that *any* call to imgui
        // before drawFrame gets accepted and we have the control over it.
        ImGui::Render();
        auto* imguiDrawData = ImGui::GetDrawData();
        auto& displaySize = imguiDrawData->DisplaySize;
        auto& displayPos = imguiDrawData->DisplayPos;
        auto& clipScale = imguiDrawData->FramebufferScale;

        // Push constants for imgui
        ImGuiPushConstants constants = {};
        constants.scale = glm::fvec2(2.0f / displaySize.x, 2.0f / displaySize.y);
        constants.translate = glm::fvec2(-1.0f - displayPos.x * constants.scale.x, -1.0f - displayPos.y * constants.scale.y);
        drawCommandBuffer->pushConstants(uiPipeline.get(), carbon::ShaderStage::Vertex, sizeof(ImGuiPushConstants), &constants);

        // Setup viewport
        drawCommandBuffer->setViewport(static_cast<float>(frameBufferWidth), static_cast<float>(frameBufferHeight), 1.0f);

        // Update vertex and index buffers
        if (imguiDrawData->TotalVtxCount > 0) {
            size_t vertexBufferSize = imguiDrawData->TotalVtxCount * sizeof(ImDrawVert);
            size_t indexBufferSize = imguiDrawData->TotalIdxCount * sizeof(ImDrawIdx);

            if (vertexBufferSize > uiVertexBuffer->getSize()) {
                // We have to allocate a new, bigger vertex buffer for the new data.
                uiVertexBuffer->destroy();

                uiVertexBuffer->create(vertexBufferSize);
                uiVertexBuffer->createDestinationBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            }

            if (indexBufferSize > uiIndexBuffer->getSize()) {
                uiIndexBuffer->destroy();

                uiIndexBuffer->create(indexBufferSize);
                uiIndexBuffer->createDestinationBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            }

            uint64_t currentVertexOffset = 0, currentIndexOffset = 0;
            for (int i = 0; i < imguiDrawData->CmdListsCount; ++i) {
                auto& list = imguiDrawData->CmdLists[i];
                uiVertexBuffer->memoryCopy(list->VtxBuffer.Data, list->VtxBuffer.Size * sizeof(ImDrawVert), currentVertexOffset);
                uiIndexBuffer->memoryCopy(list->IdxBuffer.Data, list->IdxBuffer.Size * sizeof(ImDrawIdx), currentIndexOffset);

                currentVertexOffset += list->VtxBuffer.Size * sizeof(ImDrawVert);
                currentIndexOffset += list->IdxBuffer.Size * sizeof(ImDrawIdx);
            }

            // We will now dispatch the copy commands into our draw command buffer
            uiVertexBuffer->copyIntoVram(drawCommandBuffer.get());
            uiIndexBuffer->copyIntoVram(drawCommandBuffer.get());

            // Place a buffer barrier in here, so that the draw calls are synced
            // to run after the copy has completed.
            std::array<VkMemoryBarrier, 2> memoryBarriers = {
                VkMemoryBarrier {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                },
                VkMemoryBarrier {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_INDEX_READ_BIT,
                },
            };
            drawCommandBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 2,
                                               memoryBarriers.data(), 0, nullptr, 0, nullptr);

            // Also bind the buffers so that our draw calls have data to use
            VkDeviceSize vertexBufferOffset = 0;
            drawCommandBuffer->bindVertexBuffer(uiVertexBuffer.get(), &vertexBufferOffset);
            drawCommandBuffer->bindIndexBuffer(uiIndexBuffer.get(), 0,
                                               sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
        }

        // We can now begin rendering because per the spec, one is actually not allowed to
        // call vkCmdCopyBuffer while a renderpass is running.
        // If we don't render anything the swapchain image content gets reused and
        // therefore old frames will get shown and UI elements will be shown with a trail.
        // To mitigate that issue, we clear  the image with a color; black in this case,
        // instead of loading the previous attachment.
        VkRenderingAttachmentInfo colorAttachmentInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = image->getImageView(),
            .imageLayout = image->getImageLayout(),
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .loadOp = handlesForFrame.empty() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        };
        VkRenderingInfo uiRenderingInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .flags = 0,
            .renderArea =
                VkRect2D {
                    .offset = { 0, 0 },
                    .extent = storageImage->getImageSize(),
                },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentInfo,
        };

        drawCommandBuffer->beginRendering(&uiRenderingInfo);

        uint32_t vertexOffset = 0, indexOffset = 0;
        for (size_t i = 0; i < imguiDrawData->CmdListsCount; ++i) {

            const ImDrawList* cmdList = imguiDrawData->CmdLists[i];
            for (int32_t j = 0; j < cmdList->CmdBuffer.Size; ++j) {
                auto& cmd = cmdList->CmdBuffer[j];

                glm::fvec2 clipMin((cmd.ClipRect.x - displayPos.x) * clipScale.x, (cmd.ClipRect.y - displayPos.y) * clipScale.y);
                glm::fvec2 clipMax((cmd.ClipRect.z - displayPos.x) * clipScale.x, (cmd.ClipRect.w - displayPos.y) * clipScale.y);

                VkRect2D scissor = { .offset = { .x = static_cast<int32_t>(clipMin.x), .y = static_cast<int32_t>(clipMin.y) },
                                     .extent = {
                                         .width = static_cast<uint32_t>(clipMax.x - clipMin.x),
                                         .height = static_cast<uint32_t>(clipMax.y - clipMin.y),
                                     } };
                drawCommandBuffer->setScissor(&scissor);

                drawCommandBuffer->drawIndexed(cmd.ElemCount, static_cast<int32_t>(cmd.VtxOffset + vertexOffset), 1,
                                               cmd.IdxOffset + indexOffset);
            }

            indexOffset += cmdList->IdxBuffer.Size;
            vertexOffset += cmdList->VtxBuffer.Size;
        }

        drawCommandBuffer->endRendering();
    }
    // ↑ ------------------- UI ------------------- ↑

    // Finally, move the image into a present optimal layout
    image->changeLayout(drawCommandBuffer.get(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    drawCommandBuffer->end(graphicsQueue.get());
}

void krypton::rapi::VulkanBackend::endFrame() {
    ImGui::EndFrame();

    if (needsResize)
        return;

    auto result = submitFrame();
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        needsResize = true;
    } else {
        checkResult(graphicsQueue.get(), result, "Failed to submit queue!");
    }

    auto guard = std::scoped_lock(frameHandleMutex);
    handlesForFrame.clear();
}

std::shared_ptr<krypton::rapi::CameraData> krypton::rapi::VulkanBackend::getCameraData() {
    return cameraData;
}

std::shared_ptr<krypton::rapi::Window> krypton::rapi::VulkanBackend::getWindow() {
    return window;
}

void krypton::rapi::VulkanBackend::init() {
    window->create(krypton::rapi::Backend::Vulkan);

    int width, height;
    window->getWindowSize(&width, &height);
    frameBufferWidth = width;
    frameBufferHeight = height;

    auto exts = window->getVulkanExtensions();
    for (auto& e : exts) {
        std::string ext = std::string { e };
        instanceExtensions.push_back(ext);
        krypton::log::log("Requesting window extension: {}", ext.c_str());
    }
    instance->addExtensions(instanceExtensions);
    instance->create();
    surface = window->createVulkanSurface(VkInstance(*instance));

    // Create device and allocator
    physicalDevice->create(instance.get(), surface);
    device->create(physicalDevice);

    krypton::log::log("Setting up Vulkan on: {}", physicalDevice->getDeviceName());

    // Create our VMA Allocator
    VmaAllocatorCreateInfo allocatorInfo = { .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
                                             .physicalDevice = *physicalDevice,
                                             .device = *device,
                                             .instance = *instance,
                                             .vulkanApiVersion = instance->getApiVersion() };
    vmaCreateAllocator(&allocatorInfo, &allocator);

    // Create our render sync structures
    renderFence->create(VK_FENCE_CREATE_SIGNALED_BIT);
    presentCompleteSemaphore->create();
    renderCompleteSemaphore->create();

    // Create our main graphics queue
    commandPool->create(device->getQueueIndex(vkb::QueueType::graphics), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    drawCommandBuffer = commandPool->allocateBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 0);
    device->setDebugUtilsName(*drawCommandBuffer, "drawCommandBuffer");
    graphicsQueue->create(vkb::QueueType::graphics);

    // Create the swapchain
    swapchain->create(surface, { frameBufferWidth, frameBufferHeight });

    // Initialize ImGui
    initUi();

    // Create our secondary compute queue
    blasComputeQueue->create(vkb::QueueType::compute);
    computeCommandPool->create(device->getQueueIndex(vkb::QueueType::compute), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    /* These all need the allocator in their ctor and the allocator was only constructed now,
     * therefore these are here. */
    shaderBindingTable = std::make_unique<carbon::Buffer>(device, allocator, "shaderBindingTable");
    storageImage = std::make_unique<carbon::StorageImage>(device, allocator, VkExtent2D { frameBufferWidth, frameBufferHeight });

    tlas = std::make_unique<carbon::TopLevelAccelerationStructure>(device, allocator);
    tlasInstanceBuffer = std::make_unique<carbon::StagingBuffer>(device, allocator, "tlasInstanceBuffer");
    geometryDescriptionBuffer = std::make_unique<carbon::StagingBuffer>(device, allocator, "geometryDescriptionBuffer");

    materialBuffer = std::make_unique<carbon::StagingBuffer>(device, allocator, "materialBuffer");
    materialBuffer->create(materials.size() * sizeof(krypton::assets::Material), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    materialBuffer->createDestinationBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    asProperties = std::make_unique<VkPhysicalDeviceAccelerationStructurePropertiesKHR>();
    asProperties->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    physicalDevice->getProperties(asProperties.get());

    cameraBuffer = std::make_unique<carbon::Buffer>(device, allocator, "cameraBuffer");
    cameraBuffer->create(krypton::rapi::CAMERA_DATA_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    cameraBuffer->memoryCopy(&cameraData, krypton::rapi::CAMERA_DATA_SIZE);

    // Create shaders
    using namespace krypton::shaders;
    auto shaderFile = readShaderFile("shaders/main.slang");
    auto input = std::vector { ShaderCompileInput {
        .filePath = "shaders/main.slang",
        .source = shaderFile.content,
        .entryPoints = { "raygen", "miss", "closesthit" },
        .shaderStages = { ShaderStage::RayGen, ShaderStage::Miss, ShaderStage::ClosestHit },
        .sourceType = ShaderSourceType::SLANG,
        .targetType = ShaderTargetType::SPIRV,
    } };
    auto mainShader = compileShaders(input);
    auto rahitShader = compileShaders("shaders/anyhit.rahit", ShaderStage::AnyHit, ShaderSourceType::GLSL, ShaderTargetType::SPIRV);

    if (mainShader.size() == input.front().entryPoints.size() && !rahitShader.empty()) {
        rayGenShader.shader = createShader("raygen", carbon::ShaderStage::RayGeneration, mainShader[0]);
        missShader.shader = createShader("miss", carbon::ShaderStage::RayMiss, mainShader[1]);
        closestHitShader.shader = createShader("closesthit", carbon::ShaderStage::ClosestHit, mainShader[2]);
        anyHitShader.shader = createShader("anyhit", carbon::ShaderStage::AnyHit, rahitShader.front());
    }

    // Create UI shaders
    auto uiFragShader = compileShaders("shaders/ui.frag", ShaderStage::Fragment, ShaderSourceType::GLSL, ShaderTargetType::SPIRV);
    auto uiVertShader = compileShaders("shaders/ui.vert", ShaderStage::Vertex, ShaderSourceType::GLSL, ShaderTargetType::SPIRV);

    if (!uiFragShader.empty() && !uiVertShader.empty()) {
        uiFragment = createShader("uiFragment", carbon::ShaderStage::Fragment, uiFragShader.front());
        uiVertex = createShader("uiVertex", carbon::ShaderStage::Vertex, uiVertShader.front());
    }

    window->setRapiPointer(static_cast<krypton::rapi::RenderAPI*>(this));

    /* Build necessary objects for rendering */
    storageImage->create();
    oneTimeSubmit(graphicsQueue.get(), commandPool.get(), [&](carbon::CommandBuffer* cmdBuffer) {
        storageImage->changeLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    });

    this->buildRTPipeline();
    this->buildUIPipeline();
    this->buildSBT();
}

void krypton::rapi::VulkanBackend::initUi() {
    uiVertexBuffer = std::make_unique<carbon::StagingBuffer>(device, allocator, "uiVertexBuffer");
    uiIndexBuffer = std::make_unique<carbon::StagingBuffer>(device, allocator, "uiIndexBuffer");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::GetIO().WantCaptureMouse = true;
    ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    ImGui::GetIO().BackendRendererName = "Krypton";

    window->initImgui();

    ImGui::StyleColorsDark();

    createUiFontTexture();
}

void krypton::rapi::VulkanBackend::oneTimeSubmit(carbon::Queue* queue, carbon::CommandPool* pool,
                                                 const std::function<void(carbon::CommandBuffer*)>& callback) const {
    auto queueGuard = std::move(blasComputeQueue->getLock());

    auto cmdBuffer = pool->allocateBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    cmdBuffer->begin();
    callback(cmdBuffer.get());
    cmdBuffer->end(queue);

    auto vkCmdBuffer = VkCommandBuffer(*cmdBuffer);
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

    queueGuard.unlock();

    pool->freeBuffers({ cmdBuffer.get() });
}

void krypton::rapi::VulkanBackend::render(krypton::util::Handle<"RenderObject"> handle) {
    auto lock = std::scoped_lock<std::mutex>(renderObjectMutex);
    if (objects.isHandleValid(handle)) {
        auto mutex = std::scoped_lock<std::mutex>(frameHandleMutex);
        handlesForFrame.emplace_back(handle);
    }
}

void krypton::rapi::VulkanBackend::resize(int width, int height) {
    auto result = device->waitIdle();
    checkResult(graphicsQueue.get(), result, "Failed to wait on device idle");

    frameBufferWidth = width;
    frameBufferHeight = height;

    // Re-create the swapchain.
    swapchain->create(surface, { frameBufferWidth, frameBufferHeight });

    /* Recreate the storage image */
    storageImage->recreateImage({ frameBufferWidth, frameBufferHeight });
    oneTimeSubmit(graphicsQueue.get(), commandPool.get(), [&](carbon::CommandBuffer* cmdBuffer) {
        storageImage->changeLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    });

    /* Re-bind the storage image */
    VkDescriptorImageInfo storageImageDescriptor = storageImage->getDescriptorImageInfo();
    rtDescriptorSet->updateImage(4, &storageImageDescriptor, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    needsResize = false;
}

void krypton::rapi::VulkanBackend::setObjectName(krypton::util::Handle<"RenderObject">& handle, std::string name) {
    auto lock = std::scoped_lock(renderObjectMutex);
    objects.getFromHandle(handle).name = std::move(name);
}

void krypton::rapi::VulkanBackend::setObjectTransform(krypton::util::Handle<"RenderObject">& handle, glm::mat4x3 transform) {
    auto lock = std::scoped_lock(renderObjectMutex);
    objects.getFromHandle(handle).transform = transform;
}

void krypton::rapi::VulkanBackend::shutdown() {
    checkResult(graphicsQueue.get(), device->waitIdle(), "Failed waiting on device idle");

    rayGenShader.shader->destroy();
    closestHitShader.shader->destroy();
    missShader.shader->destroy();
    anyHitShader.shader->destroy();
    uiFragment->destroy();
    uiVertex->destroy();

    uiVertexBuffer->destroy();
    uiIndexBuffer->destroy();

    uiPipeline->destroy();
    uiDescriptorSet->destroy();
    uiFontTexture->destroy();

    rtPipeline->destroy();
    rtDescriptorSet->destroy();

    cameraBuffer->destroy();
    shaderBindingTable->destroy();

    tlasInstanceBuffer->destroy();
    tlas->destroy();

    storageImage->destroy();
    swapchain->destroy();

    presentCompleteSemaphore->destroy();
    renderCompleteSemaphore->destroy();
    renderFence->destroy();

    vkDestroySurfaceKHR(*instance, surface, nullptr);

    computeCommandPool->destroy();
    commandPool->destroy();

    vmaDestroyAllocator(allocator);

    device->destroy();
    instance->destroy();
    window->destroy();
}

VkResult krypton::rapi::VulkanBackend::submitFrame() {
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };

    auto cmdBuffer = VkCommandBuffer(*drawCommandBuffer);
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

VkResult krypton::rapi::VulkanBackend::waitForFrame() {
    renderFence->wait();
    renderFence->reset();
    return swapchain->acquireNextImage(presentCompleteSemaphore, &swapchainIndex);
}

#endif // #ifdef RAPI_WITH_VULKAN
