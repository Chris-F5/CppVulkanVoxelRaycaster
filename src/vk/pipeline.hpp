#pragma once

#include <vector>

#include "vulkan/vulkan.h"

#include "descriptor_set.hpp"

struct PipelineCreateInfo
{
    VkShaderModule computeShader;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    VkPipelineShaderStageCreateFlags computeShaderStageCreateFlags;
    VkPipelineCreateFlags pipelineCreateFlags;
};

struct Pipeline
{
    VkPipelineLayout layout;
    VkPipeline pipeline;
};

Pipeline createPipeline(VkDevice device, PipelineCreateInfo info);