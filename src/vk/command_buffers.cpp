#include "command_buffers.hpp"

#include "exceptions.hpp"

VkCommandPool createCommandPool(
    VkDevice device,
    VkCommandPoolCreateFlags flags,
    uint32_t queueFamily)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.flags = flags;
    poolInfo.queueFamilyIndex = queueFamily;

    VkCommandPool commandPool;
    handleVkResult(
        vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool),
        "creating command pool");

    return commandPool;
}

void allocateCommandBuffers(
    VkDevice device,
    VkCommandPool commandPool,
    size_t count,
    VkCommandBuffer *commandBuffers)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = count;

    handleVkResult(
        vkAllocateCommandBuffers(device, &allocInfo, commandBuffers),
        "allocating command buffers");
}

void beginRecordingCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = flags;
    beginInfo.pInheritanceInfo = nullptr;

    handleVkResult(
        vkBeginCommandBuffer(commandBuffer, &beginInfo),
        "beginning command buffer recording");
}

void submitCommandBuffers(
    VkQueue queue,
    uint32_t commandBufferCount,
    VkCommandBuffer *commandBuffers,
    uint32_t waitSemaphoreCount,
    VkSemaphore *waitSemaphores,
    VkPipelineStageFlags *waitDstStageMasks,
    uint32_t signalSemaphoreCount,
    VkSemaphore *signalSemaphores,
    VkFence fence)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = waitSemaphoreCount;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitDstStageMasks;
    submitInfo.commandBufferCount = commandBufferCount;
    submitInfo.pCommandBuffers = commandBuffers;
    submitInfo.signalSemaphoreCount = signalSemaphoreCount;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(queue, 1, &submitInfo, fence);
}
