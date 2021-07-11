#include "buffer.hpp"

#include <iostream>

#include "exceptions.hpp"
#include "device.hpp"
#include "command_buffers.hpp"

void createBuffer(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize size,
    VkBufferCreateFlags flags,
    VkBufferUsageFlags usageFlags,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkBuffer *buffer,
    VkDeviceMemory *bufferMemory)
{
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = flags;
    createInfo.size = size;
    createInfo.usage = usageFlags;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount= 0;
    createInfo.pQueueFamilyIndices = nullptr;

    handleVkResult(
        vkCreateBuffer(device, &createInfo, nullptr, buffer),
        "creating buffer");
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, *buffer, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReq.memoryTypeBits, memoryPropertyFlags);

    handleVkResult(
        vkAllocateMemory(device, &allocInfo, nullptr, bufferMemory),
        "allocating buffer");

    vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

void bufferTransfer(
    VkDevice device,
    VkQueue queue,
    VkCommandPool commandPool,
    uint32_t copyRegionsCount,
    VkBufferCopy *copyRegions,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer)
{
    VkCommandBuffer commandBuffer;
    allocateCommandBuffers(
        device,
        commandPool,
        1,
        &commandBuffer
    );

    beginRecordingCommandBuffer(
        commandBuffer,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    );

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, copyRegionsCount, copyRegions);

    handleVkResult(
        vkEndCommandBuffer(commandBuffer),
        "recording buffer transfer command");

    submitCommandBuffers(
        queue,
        1,
        &commandBuffer,
        0, nullptr, nullptr,
        0, nullptr,
        VK_NULL_HANDLE
    );

    vkQueueWaitIdle(queue);
}
