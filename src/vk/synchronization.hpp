#pragma once

#include <vulkan/vulkan.hpp>

struct SynchronizationObjects
{
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
};
SynchronizationObjects createSynchronizationObjects(VkDevice device);