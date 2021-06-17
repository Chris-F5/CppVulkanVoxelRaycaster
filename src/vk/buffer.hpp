#pragma once

#include <vulkan/vulkan.h>
#include <vector>

std::vector<VkBuffer> createBuffers(
    VkDevice device,
    VkBufferCreateInfo *createInfo,
    size_t count);

std::vector<VkDeviceMemory> allocateBuffers(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    std::vector<VkBuffer> buffers,
    VkMemoryPropertyFlags memoryPropertyFlags);
