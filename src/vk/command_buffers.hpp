#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "pipeline.hpp"
#include "descriptor_set.hpp"
#include "device.hpp"

VkCommandPool createCommandPool(
    VkDevice device,
    VkCommandPoolCreateFlags flags,
    uint32_t queueFamily);

void allocateCommandBuffers(
    VkDevice device,
    VkCommandPool commandPool,
    size_t count,
    VkCommandBuffer *commandBuffers);

void recordRenderCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkPipelineLayout pipelineLayout,
    VkPipeline pipeline,
    uint32_t descriptorSetCount,
    VkDescriptorSet *descriptorSets,
    VkImage image,
    VkExtent2D imageExtent,
    uint32_t computeFamilyIndex,
    uint32_t presentFamilyIndex);

void recordTransferCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer src,
    VkBuffer dst,
    VkDeviceSize size);
