#pragma once

#include <vector>

#include <vulkan/vulkan.h>

struct DescriptorCreateInfo
{
    uint32_t binding;
    VkDescriptorType type;
    VkShaderStageFlags stageFlags;

    std::vector<VkImageView> imageViews;
    VkImageLayout imageLayout;

    std::vector<VkBuffer> buffers;
};

struct DescriptorSets
{
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    std::vector<VkDescriptorSet> sets;
};

DescriptorSets createDescriptorSets(
    VkDevice device,
    std::vector<DescriptorCreateInfo> descriptorInfos,
    uint32_t setCount);
