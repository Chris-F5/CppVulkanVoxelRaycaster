#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>

struct QueueFamilyIndices
{
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return computeFamily.has_value() && presentFamily.has_value();
    }
};

VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow *window);
VkInstance createInstance(const bool enableValidationLayers);
VkPhysicalDevice pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, bool enableValidationLayers, QueueFamilyIndices queueFamilyIndices);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);