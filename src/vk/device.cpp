#include "device.hpp"

#include <iostream>
#include <vector>
#include <cstring>
#include <set>

#include "swapchain.hpp"
#include "exceptions.hpp"

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow *window)
{
    VkSurfaceKHR surface;
    handleVkResult(
        glfwCreateWindowSurface(instance, window, nullptr, &surface),
        "creating surface");
    return surface;
}

bool checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

VkInstance createInstance(const bool enableValidationLayers)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulcan Test App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    if (enableValidationLayers)
    {
        if (!checkValidationLayerSupport())
        {
            throw std::runtime_error("Validation layers enabled but not supported");
        }
        else
        {
            std::cout << "Using validation layers" << std::endl;
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    uint32_t extentionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extentionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extentionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extentionCount, extensions.data());

    VkInstance instance;
    handleVkResult(
        vkCreateInstance(&createInfo, nullptr, &instance),
        "creating vulkan instance");

    return instance;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> avaliableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, avaliableExtensions.data());
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : avaliableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}

bool isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    QueueFamilyInfo computeAndPresentFamily = pickComputeAndPresentFamily(physicalDevice, surface);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           computeAndPresentFamily.found &&
           checkDeviceExtensionSupport(physicalDevice) &&
           checkSwapchainSupport(physicalDevice, surface);
}

VkPhysicalDevice pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    if (physicalDeviceCount == 0)
    {
        throw std::runtime_error("failed to find any GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
    for (const auto &physicalDevice : physicalDevices)
    {
        if (isPhysicalDeviceSuitable(physicalDevice, surface))
        {
            return physicalDevice;
        }
    }

    throw std::runtime_error("failed to find suitable GPU");
}

VkDevice createLogicalDevice(
    VkPhysicalDevice physicalDevice,
    bool enableValidationLayers,
    uint32_t queueFamilyIndiciesCount,
    uint32_t *queueFamilyIndices)
{
    VkDeviceQueueCreateInfo *queueCreateInfos = 
        (VkDeviceQueueCreateInfo*)malloc(queueFamilyIndiciesCount * sizeof(VkDeviceQueueCreateInfo));

    float queuePriority = 1.0f;
    for (int i = 0; i < queueFamilyIndiciesCount; i++)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndices[i];
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[i] = queueCreateInfo;
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = queueFamilyIndiciesCount;
    createInfo.pQueueCreateInfos = queueCreateInfos;

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VkDevice device;
    handleVkResult(
        vkCreateDevice(physicalDevice, &createInfo, nullptr, &device),
        "creating logical device");
    
    free(queueCreateInfos);

    return device;
}

uint32_t scoreComputeAndPresentFamily(VkPhysicalDevice physicalDevice, uint32_t index, VkQueueFamilyProperties properties, VkSurfaceKHR surface){
    if ((properties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)
        return 0;

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &presentSupport);
    if (!presentSupport)
        return 0;

    uint32_t score = 1;
    if((properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
        score += 3;
    if((properties.queueFlags & VK_QUEUE_TRANSFER_BIT) == 0)
        score += 1;
    if((properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) == 0)
        score += 1;
    return score;
}

QueueFamilyInfo pickComputeAndPresentFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface){
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    VkQueueFamilyProperties *queueFamilyProperties = 
        (VkQueueFamilyProperties*) malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties);

    uint32_t bestScore = 0;
    uint32_t bestIndex = 0;

    for(int i = 0; i < queueFamilyCount; i++){
        uint score = scoreComputeAndPresentFamily(physicalDevice, i, queueFamilyProperties[i], surface);
        if (score > bestScore){
            bestScore = score;
            bestIndex = i;
        }
    }
    free(queueFamilyProperties);

    QueueFamilyInfo info{};
    info.found = bestScore != 0;
    info.index = bestIndex;
    info.score = bestScore;
    return info;
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (size_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type");
}
