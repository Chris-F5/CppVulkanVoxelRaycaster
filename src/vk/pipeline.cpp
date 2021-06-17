#include "pipeline.hpp"

#include <iostream>

#include "exceptions.hpp"

VkPipelineLayout createPipelineLayout(VkDevice device, const PipelineCreateInfo *info)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pNext = nullptr;
    // flags is reserved for future use
    pipelineLayoutInfo.flags = 0;
    pipelineLayoutInfo.setLayoutCount = info->descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = info->descriptorSetLayouts.data();
    // TODO: support push constants
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VkPipelineLayout pipelineLayout;
    handleVkResult(
        vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout),
        "creating pipeline layout");

    return pipelineLayout;
}

VkPipeline createComputePipeline(VkDevice device, VkPipelineLayout pipelineLayout, const PipelineCreateInfo *info)
{
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.pNext = nullptr;
    shaderStageInfo.flags = info->computeShaderStageCreateFlags;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = info->computeShader;
    shaderStageInfo.pName = "main";
    // TODO: support shader specialization
    shaderStageInfo.pSpecializationInfo = nullptr;

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = info->pipelineCreateFlags;
    pipelineInfo.stage = shaderStageInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.basePipelineHandle = nullptr;

    VkPipeline pipeline;
    handleVkResult(
        vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
        "creating compute pipeline");

    return pipeline;
}

Pipeline createPipeline(VkDevice device, PipelineCreateInfo info)
{
    VkPipelineLayout layout = createPipelineLayout(device, &info);
    VkPipeline vkPipeline = createComputePipeline(device, layout, &info);

    Pipeline pipeline{};
    pipeline.layout = layout;
    pipeline.pipeline = vkPipeline;

    return pipeline;
}
