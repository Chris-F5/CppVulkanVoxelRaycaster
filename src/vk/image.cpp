#include "image.hpp"

#include "exceptions.hpp"
#include "device.hpp"
#include "command_buffers.hpp"

VkImageSubresourceRange createImageSubresourceRange(
    VkImageAspectFlags aspectMask,
    uint32_t baseMipLevel,
    uint32_t mipLevelCount,
    uint32_t baseArrayLayer,
    uint32_t arrayLayerCount)
{
    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = baseMipLevel;
    subresourceRange.levelCount = mipLevelCount;
    subresourceRange.baseArrayLayer = baseArrayLayer;
    subresourceRange.layerCount = arrayLayerCount;

    return subresourceRange;
}

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
    VkDeviceMemory *imageMemory)
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
    createInfo.usage = usageFlags;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.initialLayout = preinitialized? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;

    handleVkResult(
        vkCreateImage(device, &createInfo, nullptr, image),
        "creating image");

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device, *image, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReq.memoryTypeBits, memoryPropertyFlags);
    
    handleVkResult(
        vkAllocateMemory(device, &allocInfo, nullptr, imageMemory),
        "allocating image");

    vkBindImageMemory(device, *image, *imageMemory, 0);
}

VkImageView createImageView(
    VkDevice device,
    VkImage image,
    VkFormat format,
    VkImageViewType viewType,
    VkImageSubresourceRange imageSubresourceRange)
{
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
    createInfo.subresourceRange = imageSubresourceRange;

    VkImageView imageView;
    handleVkResult(
        vkCreateImageView(device, &createInfo, nullptr, &imageView),
        "creating image view");
    
    return imageView;
}

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
    VkImage image)
{
    VkCommandBuffer commandBuffer;
    allocateCommandBuffers(device, commandPool, 1, &commandBuffer);

    beginRecordingCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkImageSubresourceLayers imageSubresource{};
    imageSubresource.aspectMask = aspectMask;
    imageSubresource.mipLevel = mipLevel;
    imageSubresource.baseArrayLayer = baseArrayLayer;
    imageSubresource.layerCount = arrayLayerCount;

    VkBufferImageCopy bufferImageCopy{};
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;
    bufferImageCopy.imageSubresource = imageSubresource;
    bufferImageCopy.imageOffset = imageOffset;
    bufferImageCopy.imageExtent = imageExtent;

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);

    vkEndCommandBuffer(commandBuffer);

    submitCommandBuffers(
        queue,
        1,
        &commandBuffer,
        0, nullptr, nullptr,
        0, nullptr, 
        VK_NULL_HANDLE
    );

    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

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
    VkPipelineStageFlags dstStageMask)
{
    VkCommandBuffer commandBuffer;
    allocateCommandBuffers(device, commandPool, 1, &commandBuffer);

    beginRecordingCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = imageSubresourceRange;

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStageMask, dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    vkEndCommandBuffer(commandBuffer);

    submitCommandBuffers(
        queue,
        1,
        &commandBuffer,
        0, nullptr, nullptr,
        0, nullptr, 
        VK_NULL_HANDLE
    );

    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
