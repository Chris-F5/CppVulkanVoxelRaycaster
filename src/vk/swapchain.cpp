#include "swapchain.hpp"

#include <iostream>

#include "exceptions.hpp"
#include "image.hpp"

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
};

SwapchainSupportDetails querySupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.surfaceCapabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    details.surfaceFormats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.surfaceFormats.data());

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());

    return details;
}

VkSurfaceFormatKHR sellectSurfaceFormat(std::vector<VkSurfaceFormatKHR> availableFormats)
{
    if (availableFormats.empty())
    {
        throw std::invalid_argument("Cant sellect surface format from emply vector.");
    }

    for (const auto &availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR sellectPresentMode(std::vector<VkPresentModeKHR> availablePresentModes)
{
    for (const auto &availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, GLFWwindow *window)
{
    if (surfaceCapabilities.currentExtent.width == UINT32_MAX)
    {
        // Surface size will be determined by the extent of a swapchain targeting the surface
        std::cout << "Surface did not specify its own extent. Picking a suitable one." << std::endl;

        int width,
            height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)};

        actualExtent.width = std::max(
            surfaceCapabilities.minImageExtent.width,
            std::min(
                surfaceCapabilities.maxImageExtent.width,
                actualExtent.width));

        actualExtent.height = std::max(
            surfaceCapabilities.minImageExtent.height,
            std::min(
                surfaceCapabilities.maxImageExtent.height,
                actualExtent.height));

        return actualExtent;
    }
    else
    {
        return surfaceCapabilities.currentExtent;
    }
}

uint32_t chooseImageCount(VkSurfaceCapabilitiesKHR surfaceCapabilities)
{
    uint32_t count = surfaceCapabilities.minImageCount + 1;

    if (surfaceCapabilities.maxImageCount != 0)
    {
        count = std::min(count, surfaceCapabilities.maxImageCount);
    }

    return count;
}

std::vector<VkImage> getSwapchainImages(VkDevice device, VkSwapchainKHR swapchain)
{
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

    return images;
}

bool checkSwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SwapchainSupportDetails supportDetails = querySupportDetails(physicalDevice, surface);
    return !supportDetails.surfaceFormats.empty() &&
           !supportDetails.presentModes.empty();
}

Swapchain createSwapchain(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    GLFWwindow *window,
    VkSurfaceKHR surface)
{
    Swapchain swapchain;

    SwapchainSupportDetails supportDetails = querySupportDetails(physicalDevice, surface);

    swapchain.currentFrame = 0;
    swapchain.presentMode = sellectPresentMode(supportDetails.presentModes);
    swapchain.surfaceFormat = sellectSurfaceFormat(supportDetails.surfaceFormats);
    swapchain.extent = chooseExtent(supportDetails.surfaceCapabilities, window);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    // TODO: Try setting this to 2
    createInfo.minImageCount = chooseImageCount(supportDetails.surfaceCapabilities);
    createInfo.imageFormat = swapchain.surfaceFormat.format;
    createInfo.imageColorSpace = swapchain.surfaceFormat.colorSpace;
    createInfo.imageExtent = swapchain.extent;
    // Number of layers each image consists of. 1 because we render to a 2d screen.
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = supportDetails.surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = swapchain.presentMode;
    // If another window is obscuring part of this image, we can clip it.
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    handleVkResult(
        vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain.swapchain),
        "creating swapchain");

    swapchain.images = getSwapchainImages(device, swapchain.swapchain);
    swapchain.imageViews = std::vector<VkImageView>(swapchain.imageCount());
    for(int i = 0 ; i < swapchain.imageCount(); i++)
        swapchain.imageViews[i] = createImageView(
            device,
            swapchain.images[i],
            swapchain.surfaceFormat.format,
            VK_IMAGE_VIEW_TYPE_2D);
    return swapchain;
}

void cleanupSwapchain(VkDevice device, Swapchain swapchain)
{
    for (auto imageView : swapchain.imageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);
}