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