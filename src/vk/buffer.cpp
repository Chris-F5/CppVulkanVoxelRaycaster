#include "buffer.hpp"

#include <iostream>

#include "exceptions.hpp"

void createBuffers(
    VkDevice device,
    VkBufferCreateFlags flags,
    VkDeviceSize size,
    VkBufferUsageFlags usageFlags,
    size_t count,
    VkBuffer *buffers)
{
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = flags;
    createInfo.size = size;
    createInfo.usage = usageFlags;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    for (size_t i = 0; i < count; i++)
    {
        handleVkResult(
            vkCreateBuffer(device, &createInfo, nullptr, &buffers[i]),
            "creating buffer");
    }
}


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

void allocateBuffers(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    uint32_t count,
    VkBuffer *buffers,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkDeviceMemory *buffersMemory)
{
    for (size_t i = 0; i < count; i++)
    {
        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, buffers[i], &memReq);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReq.memoryTypeBits, memoryPropertyFlags);

        handleVkResult(
            vkAllocateMemory(device, &allocInfo, nullptr, &buffersMemory[i]),
            "allocating buffer");

        vkBindBufferMemory(device, buffers[i], buffersMemory[i], 0);
    }
}
