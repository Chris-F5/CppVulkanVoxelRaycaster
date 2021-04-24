#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

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

    void cleanup(VkDevice device);
};

Swapchain createSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, GLFWwindow *window, VkSurfaceKHR surface, uint32_t queueFamilies[], int queueFamilyCount);
bool checkSwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);