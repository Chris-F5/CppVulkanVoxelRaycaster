#pragma once

#include <vulkan/vulkan.h>

void createBuffers(
    VkDevice device,
    VkBufferCreateFlags flags,
    VkDeviceSize size,
    VkBufferUsageFlags usageFlags,
    size_t count,
    VkBuffer *buffers);

void allocateBuffers(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    uint32_t count,
    VkBuffer *buffers,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkDeviceMemory *buffersMemory);
