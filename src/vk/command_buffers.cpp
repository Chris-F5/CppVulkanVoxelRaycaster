#include "command_buffers.hpp"

#include "exceptions.hpp"

VkCommandPool createCommandPool(
    VkDevice device,
    VkCommandPoolCreateFlags flags,
    uint32_t queueFamily)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.flags = flags;
    poolInfo.queueFamilyIndex = queueFamily;

    VkCommandPool commandPool;
    handleVkResult(
        vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool),
        "creating command pool");

    return commandPool;
}

void allocateCommandBuffers(
    VkDevice device,
    VkCommandPool commandPool,
    size_t count,
    VkCommandBuffer *commandBuffers)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = count;

    handleVkResult(
        vkAllocateCommandBuffers(device, &allocInfo, commandBuffers),
        "allocating command buffers");
}

void recordRenderCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkPipelineLayout pipelineLayout,
    VkPipeline pipeline,
    uint32_t descriptorSetCount,
    VkDescriptorSet *descriptorSets,
    VkImage image,
    VkExtent2D imageExtent,
    uint32_t computeFamilyIndex,
    uint32_t presentFamilyIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    handleVkResult(
        vkBeginCommandBuffer(commandBuffer, &beginInfo),
        "beginning command buffer recording");

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipelineLayout,
        0,
        descriptorSetCount,
        descriptorSets,
        0,
        nullptr);

    VkImageSubresourceRange imageRange{};
    imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageRange.baseMipLevel = 0;
    imageRange.levelCount = VK_REMAINING_MIP_LEVELS;
    imageRange.baseArrayLayer = 0;
    imageRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkImageMemoryBarrier preImageBarrier{};
    preImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    preImageBarrier.srcAccessMask = 0;
    preImageBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    preImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    preImageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    preImageBarrier.srcQueueFamilyIndex = presentFamilyIndex;
    preImageBarrier.dstQueueFamilyIndex = computeFamilyIndex;
    preImageBarrier.image = image;
    preImageBarrier.subresourceRange = imageRange;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &preImageBarrier);
    
    vkCmdDispatch(commandBuffer, imageExtent.width, imageExtent.height, 1);

    VkImageMemoryBarrier postImageBarrier{};
    postImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    postImageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    postImageBarrier.dstAccessMask = 0;
    postImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    postImageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    postImageBarrier.srcQueueFamilyIndex = computeFamilyIndex;
    postImageBarrier.dstQueueFamilyIndex = presentFamilyIndex;
    postImageBarrier.image = image;
    postImageBarrier.subresourceRange = imageRange;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &postImageBarrier);

    handleVkResult(
        vkEndCommandBuffer(commandBuffer),
        "ending commanbuffersd buffer recording");
}

void recordTransferCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer src,
    VkBuffer dst,
    VkDeviceSize size)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1 , &copyRegion);

    vkEndCommandBuffer(commandBuffer);
}
