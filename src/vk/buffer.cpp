#include <iostream>

#include "buffer.hpp"

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (size_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type");
}

std::vector<VkDeviceMemory> allocateBuffers(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    std::vector<VkBuffer> buffers,
    VkMemoryPropertyFlags memoryPropertyFlags)
{
    std::vector<VkDeviceMemory> buffersMemory(buffers.size());
    for (size_t i = 0; i < buffers.size(); i++)
    {
        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, buffers[i], &memReq);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReq.memoryTypeBits, memoryPropertyFlags);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &buffersMemory[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate buffer");
        }

        vkBindBufferMemory(device, buffers[i], buffersMemory[i], 0);
    }
    return buffersMemory;
}

std::vector<VkBuffer> createBuffers(
    VkDevice device,
    VkBufferCreateInfo *createInfo,
    size_t count)
{
    std::vector<VkBuffer> buffers(count);

    for (size_t i = 0; i < count; i++)
    {
        if (vkCreateBuffer(device, createInfo, nullptr, &buffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create buffer");
        }
    }
    return buffers;
}
