#pragma once

#include <vulkan/vulkan.h>

void createBuffer(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize size,
    VkBufferCreateFlags flags,
    VkBufferUsageFlags usageFlags,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkBuffer *buffer,
    VkDeviceMemory *bufferMemory);

void bufferTransfer(
    VkDevice device,
    VkQueue queue,
    VkCommandPool commandPool,
    uint32_t copyRegionsCount,
    VkBufferCopy *copyRegions,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer);
