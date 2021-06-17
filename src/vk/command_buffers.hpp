#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "pipeline.hpp"
#include "descriptor_set.hpp"
#include "device.hpp"

struct CommandBuffersCreateInfo
{
    size_t count;
    QueueFamilyIndices queueFamilies;
    VkCommandPoolCreateFlags poolCreateFlags;
    VkCommandBufferUsageFlags usageFlags;
    Pipeline pipeline;
    std::vector<DescriptorSets> descriptorSets;
    std::vector<VkImage> images;
    VkExtent2D imageExtent;
};

struct CommandBuffers
{
    VkCommandPool pool;
    std::vector<VkCommandBuffer> buffers;
};

CommandBuffers createCommandBuffers(VkDevice device, CommandBuffersCreateInfo info);
