#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>

struct QueueFamilyInfo{
    bool found;
    uint32_t index;
    uint32_t score;
};

VkInstance createInstance(const bool enableValidationLayers);
VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow *window);
VkPhysicalDevice pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
VkDevice createLogicalDevice(
    VkPhysicalDevice physicalDevice,
    bool enableValidationLayers,
    uint32_t queueFamilyIndiciesCount,
    uint32_t *queueFamilyIndices);
QueueFamilyInfo pickComputeAndPresentFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
