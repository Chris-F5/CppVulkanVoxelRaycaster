#pragma once

#include <vulkan/vulkan.h>

void createImage(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkImageType imageType,
    VkFormat format,
    VkExtent3D extent,
    VkImageViewCreateFlags flags,
    VkImageUsageFlags usageFlags,
    uint32_t mipLevels,
    uint32_t arrayLayers,
    VkSampleCountFlagBits samples,
    VkImageTiling tiling,
    bool preinitialized,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkImage *image,
    VkDeviceMemory *imageMemory);

VkImageView createImageView(
    VkDevice device,
    VkImage image,
    VkFormat format,
    VkImageViewType viewType);

