#include "buffer.hpp"

#include <iostream>

#include "exceptions.hpp"

VkBuffer createBuffer(
    VkDevice device,
    VkBufferCreateFlags flags,
    VkDeviceSize size,
    VkBufferUsageFlags usageFlags)
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

    VkBuffer buffer;
    handleVkResult(
        vkCreateBuffer(device, &createInfo, nullptr, &buffer),
        "creating buffer");
    return buffer;
}

VkImage createImage(
    VkDevice device,
    VkImageViewCreateFlags flags,
    VkImageType imageType,
    VkFormat format,
    VkExtent3D extent,
    uint32_t mipLevels,
    uint32_t arrayLayers,
    VkSampleCountFlagBits samples,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkImageLayout initialLayout)
{
    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = flags;
    createInfo.imageType = imageType;
    createInfo.format = format;
    createInfo.extent = extent;
    createInfo.mipLevels = mipLevels;
    createInfo.arrayLayers = arrayLayers;
    createInfo.samples = samples;
    createInfo.tiling = tiling;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    // initialLayout must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED
    createInfo.initialLayout = initialLayout;

    VkImage image;
    handleVkResult(
        vkCreateImage(device, &createInfo, nullptr, &image),
        "creating image");

    return image;
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

VkDeviceMemory allocateBuffer(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkBuffer buffer,
    VkMemoryPropertyFlags memoryPropertyFlags)
{
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, buffer, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReq.memoryTypeBits, memoryPropertyFlags);

    VkDeviceMemory bufferMemory;
    handleVkResult(
        vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory),
        "allocating buffer");

    vkBindBufferMemory(device, buffer, bufferMemory, 0);

    return bufferMemory;
}

VkDeviceMemory allocateImage(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkImage image,
    VkMemoryPropertyFlags memoryPropertyFlags)
{
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device, image, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReq.memoryTypeBits, memoryPropertyFlags);

    VkDeviceMemory imageMemory;
    handleVkResult(
        vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory),
        "allocating image");

    vkBindImageMemory(device, image, imageMemory, 0);

    return imageMemory;    
}

VkImageView createImageView(
    VkDevice device,
    VkImage image,
    VkFormat format,
    VkImageViewType viewType)
{
    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.image = image;
    createInfo.viewType = viewType;
    createInfo.format = format;
    createInfo.components = VkComponentMapping {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY};
    createInfo.subresourceRange = subresourceRange;

    VkImageView imageView;
    handleVkResult(
        vkCreateImageView(device, &createInfo, nullptr, &imageView),
        "creating image view");
    
    return imageView;
}
