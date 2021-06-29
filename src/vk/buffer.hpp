#pragma once

#include <vulkan/vulkan.h>

VkBuffer createBuffer(
    VkDevice device,
    VkBufferCreateFlags flags,
    VkDeviceSize size,
    VkBufferUsageFlags usageFlags);

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
    VkImageLayout initialLayout);

VkDeviceMemory allocateBuffer(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkBuffer buffer,
    VkMemoryPropertyFlags memoryPropertyFlags);

VkDeviceMemory allocateImage(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkImage image,
    VkMemoryPropertyFlags memoryPropertyFlags);

VkImageView createImageView(
    VkDevice device,
    VkImage image,
    VkFormat format,
    VkImageViewType viewType);
