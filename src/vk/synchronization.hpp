#pragma once

#include <vulkan/vulkan.hpp>

VkSemaphore createSemaphore(VkDevice device);
VkFence createFence(VkDevice device, VkFenceCreateFlags flags);
