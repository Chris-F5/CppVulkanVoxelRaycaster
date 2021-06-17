#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "device.hpp"

struct Swapchain
{
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkExtent2D extent;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    size_t currentFrame;

    uint32_t imageCount() { return images.size(); };
};

bool checkSwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
Swapchain createSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, GLFWwindow *window, VkSurfaceKHR surface, QueueFamilyIndices queueFamilyIndices);
void cleanupSwapchain(VkDevice device, Swapchain swapchain);