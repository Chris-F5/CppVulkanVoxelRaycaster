#include "image.hpp"

#include "exceptions.hpp"
#include "device.hpp"

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
