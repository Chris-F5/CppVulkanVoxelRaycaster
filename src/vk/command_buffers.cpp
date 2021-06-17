#include "command_buffers.hpp"

#include "exceptions.hpp"

VkCommandPool createComputeCommandPool(VkDevice device, CommandBuffersCreateInfo *info)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.flags = info->poolCreateFlags;
    poolInfo.queueFamilyIndex = info->queueFamilies.computeFamily.value();

    VkCommandPool commandPool;
    handleVkResult(
        vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool),
        "creating command pool");

    return commandPool;
}

std::vector<VkCommandBuffer> allocateCommandBuffers(
    VkDevice device,
    VkCommandPool commandPool,
    CommandBuffersCreateInfo *info)
{
    std::vector<VkCommandBuffer> commandBuffers(info->count);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = info->count;

    handleVkResult(
        vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()),
        "allocating command buffers");
    return commandBuffers;
}

void recordCommandBuffer(
    VkDevice device,
    VkCommandBuffer commandBuffer,
    CommandBuffersCreateInfo *info,
    size_t bufferIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = info->usageFlags;
    beginInfo.pInheritanceInfo = nullptr;

    std::vector<VkDescriptorSet> descriptorSetToBind(info->descriptorSets.size());
    for (int i = 0; i < info->descriptorSets.size(); i++)
    {
        descriptorSetToBind[i] = info->descriptorSets[i].sets[bufferIndex];
    }

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
    preImageBarrier.srcQueueFamilyIndex = info->queueFamilies.presentFamily.value();
    preImageBarrier.dstQueueFamilyIndex = info->queueFamilies.computeFamily.value();
    preImageBarrier.image = info->images[bufferIndex];
    preImageBarrier.subresourceRange = imageRange;

    VkImageMemoryBarrier postImageBarrier{};
    postImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    postImageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    postImageBarrier.dstAccessMask = 0;
    postImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    postImageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    postImageBarrier.srcQueueFamilyIndex = info->queueFamilies.computeFamily.value();
    postImageBarrier.dstQueueFamilyIndex = info->queueFamilies.presentFamily.value();
    postImageBarrier.image = info->images[bufferIndex];
    postImageBarrier.subresourceRange = imageRange;

    handleVkResult(
        vkBeginCommandBuffer(commandBuffer, &beginInfo),
        "beginning command buffer recording");

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, info->pipeline.pipeline);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        info->pipeline.layout,
        0,
        descriptorSetToBind.size(),
        descriptorSetToBind.data(),
        0,
        nullptr);

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &preImageBarrier);

    vkCmdDispatch(commandBuffer, info->imageExtent.width, info->imageExtent.height, 1);

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &postImageBarrier);

    handleVkResult(
        vkEndCommandBuffer(commandBuffer),
        "ending command buffer recording");
}

CommandBuffers createCommandBuffers(VkDevice device, CommandBuffersCreateInfo info)
{
    VkCommandPool pool = createComputeCommandPool(device, &info);
    std::vector<VkCommandBuffer> vkCommandBuffers = allocateCommandBuffers(device, pool, &info);
    for (int i = 0; i < info.count; i++)
    {
        recordCommandBuffer(device, vkCommandBuffers[i], &info, i);
    }

    CommandBuffers commandBuffers{};
    commandBuffers.pool = pool;
    commandBuffers.buffers = vkCommandBuffers;

    return commandBuffers;
}
