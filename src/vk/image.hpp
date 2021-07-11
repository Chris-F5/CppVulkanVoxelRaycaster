#pragma once

#include <vulkan/vulkan.h>

VkImageSubresourceRange createImageSubresourceRange(
    VkImageAspectFlags aspectMask,
    uint32_t baseMipLevel,
    uint32_t mipLevelCount,
    uint32_t baseArrayLayer,
    uint32_t arrayLayerCount);

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
    VkImageViewType viewType,
    VkImageSubresourceRange imageSubresourceRange);

void copyBufferToImage(
    VkDevice device,
    VkQueue queue,
    VkCommandPool commandPool,
    VkOffset3D imageOffset,
    VkExtent3D imageExtent,
    VkImageAspectFlags aspectMask,
    uint32_t mipLevel,
    uint32_t baseArrayLayer,
    uint32_t arrayLayerCount,
    VkBuffer buffer,
    VkImage image);

void transitionImageLayout(
    VkDevice device,
    VkQueue queue,
    VkCommandPool commandPool,
    VkImage image,
    VkImageSubresourceRange imageSubresourceRange,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkPipelineStageFlags srcStageMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags dstStageMask);
