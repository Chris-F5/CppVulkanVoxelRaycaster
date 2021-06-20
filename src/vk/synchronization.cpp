#include "synchronization.hpp"

#include "exceptions.hpp"

VkSemaphore createSemaphore(VkDevice device)
{
    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;

    VkSemaphore semaphore;
    handleVkResult(
        vkCreateSemaphore(device, &createInfo, nullptr, &semaphore),
        "creating semaphore");

    return semaphore;
}

VkFence createFence(VkDevice device, VkFenceCreateFlags flags){
    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = flags;
    VkFence fence;
    handleVkResult(
        vkCreateFence(device, &createInfo, nullptr, &fence),
        "creating fence");
    return fence;
}