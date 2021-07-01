#include "buffer.hpp"

#include <iostream>

#include "exceptions.hpp"
#include "device.hpp"

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
