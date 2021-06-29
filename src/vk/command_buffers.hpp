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

void beginRecordingCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkCommandBufferUsageFlags flags);

void submitCommandBuffers(
    VkQueue queue,
    uint32_t commandBufferCount,
    VkCommandBuffer *commandBuffers,
    uint32_t waitSemaphoreCount,
    VkSemaphore *waitSemaphores,
    VkPipelineStageFlags *waitDstStageMasks,
    uint32_t signalSemaphoreCount,
    VkSemaphore *signalSemaphores,
    VkFence fence);
