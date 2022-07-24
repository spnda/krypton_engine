#include <Tracy.hpp>

#include <rapi/metal/metal_command_buffer.hpp>
#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_queue.hpp>
#include <rapi/metal/metal_sync.hpp>

namespace kr = krypton::rapi;

kr::mtl::Queue::Queue(MTL::Device* device, MTL::CommandQueue* queue) : device(device), queue(queue) {}

std::shared_ptr<kr::ICommandBufferPool> kr::mtl::Queue::createCommandPool() {
    return std::make_shared<kr::mtl::CommandBufferPool>(device, this);
}

MTL::CommandQueue* kr::mtl::Queue::getHandle() const {
    return queue;
}

void kr::mtl::Queue::setName(std::string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    queue->setLabel(name);
}

void kr::mtl::Queue::submit(ICommandBuffer* cmdBuffer, ISemaphore* wait, ISemaphore* signal, IFence* fence) {
    auto buffer = dynamic_cast<CommandBuffer*>(cmdBuffer)->buffer;
    if (wait != nullptr)
        dynamic_cast<Semaphore*>(wait)->wait();
    buffer->addCompletedHandler([fence, signal](auto* cmdBuffer) {
        if (signal != nullptr)
            dynamic_cast<Semaphore*>(signal)->signal();
        if (fence != nullptr)
            dynamic_cast<Fence*>(fence)->signal();
    });
    buffer->commit();
}
