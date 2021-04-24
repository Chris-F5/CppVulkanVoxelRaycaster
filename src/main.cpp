#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>

#include "vulkan_device.hpp"
#include "swapchain.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

static std::vector<char> readFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return computeFamily.has_value() && presentFamily.has_value();
    }
};

bool rightArrowDown = false;
bool leftArrowDown = false;
bool upArrowDown = false;
bool downArrowDown = false;

void keyCallback(GLFWwindow *_window, int key, int _scanCode, int action, int _mods)
{
    bool isPress = action == GLFW_PRESS;
    bool isRelease = action == GLFW_RELEASE;
    if (isPress || isRelease)
    {
        switch (key)
        {
        case GLFW_KEY_D:
            rightArrowDown = isPress;
            break;
        case GLFW_KEY_A:
            leftArrowDown = isPress;
            break;
        case GLFW_KEY_W:
            upArrowDown = isPress;
            break;
        case GLFW_KEY_S:
            downArrowDown = isPress;
            break;
        }
    }
}

class App
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow *window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    QueueFamilyIndices queueFamilyIndices;
    VkQueue computeQueue;
    VkQueue presentQueue;
    Swapchain swapchain;
    VkPipelineLayout pipelineLayout;
    VkPipeline computePipeline;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;
    glm::vec3 cameraPosition = glm::vec3(0.0);
    std::vector<VkBuffer> camInfoBuffers;
    std::vector<VkDeviceMemory> camInfoBuffersMemory;

    void initWindow()
    {
        glfwInit();

        // Dont use opengl
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulcan Test App", nullptr, nullptr);

        glfwSetKeyCallback(window, keyCallback);
    }

    void initVulkan()
    {
        instance = createInstance(enableValidationLayers);
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        queueFamilyIndices = findQueueFamilies(physicalDevice);
        uint32_t swapchainAccessQueueFamilies[] = {
            queueFamilyIndices.computeFamily.value(),
            queueFamilyIndices.presentFamily.value()};
        swapchain = createSwapchain(
            device,
            physicalDevice,
            window,
            surface,
            swapchainAccessQueueFamilies,
            sizeof(swapchainAccessQueueFamilies) / sizeof(swapchainAccessQueueFamilies[0]));
        //createRenderPass();
        createComputePipeline();
        //createFramebuffers();
        createCommandPool();
        // Bind it as a storage image
        createCommandBuffers();
        createSyncObjects();
    }

    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface");
        }
    }

    void pickPhysicalDevice()
    {
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
        if (physicalDeviceCount == 0)
        {
            throw std::runtime_error("Failed to find any GPUs with Vulcan support");
        }

        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

        for (const auto &physicalDevice : physicalDevices)
        {
            if (isPhysicalDeviceSuitable(physicalDevice))
            {
                this->physicalDevice = physicalDevice;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Failed to find suitable GPU");
        }
    }

    bool isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;

        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
               queueFamilyIndices.isComplete() &&
               checkDeviceExtensionSupport(physicalDevice) &&
               checkSwapchainSupport(physicalDevice, surface);
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        uint32_t i = 0;
        for (const auto &queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            { // TODO: Check if not graphics
                indices.computeFamily = i;
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                indices.presentFamily = i;
            }
            if (indices.isComplete())
            {
                break;
            }
            i++;
        }

        return indices;
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.computeFamily.value(),
            indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

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

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device");
        }

        vkGetDeviceQueue(device, indices.computeFamily.value(), 0, &computeQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
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

    VkShaderModule createShaderModule(const std::vector<char> &spv)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = spv.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(spv.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create shader module");
        }
        return shaderModule;
    }

    void createComputePipeline()
    {
        auto shaderSpv = readFile("shader.spv");

        VkShaderModule shaderModule = createShaderModule(shaderSpv);

        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = shaderModule;
        shaderStageInfo.pName = "main";

        VkDescriptorSetLayoutBinding imageBinding{};
        imageBinding.binding = 0;
        imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        imageBinding.descriptorCount = 1;
        imageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding camInfoBinding{};
        camInfoBinding.binding = 1;
        camInfoBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        camInfoBinding.descriptorCount = 1;
        camInfoBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding descriptorBindings[] = {imageBinding, camInfoBinding};

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = 2;
        descriptorSetLayoutInfo.pBindings = descriptorBindings;

        if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor set layout");
        }

        // ===== Create descriptor set layouts for images and cam info

        VkDescriptorPoolSize imagePoolSize{};
        imagePoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        imagePoolSize.descriptorCount = static_cast<uint32_t>(swapchain.imageCount());

        VkDescriptorPoolSize camInfoPoolSize{};
        camInfoPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        camInfoPoolSize.descriptorCount = static_cast<uint32_t>(swapchain.imageCount());

        VkDescriptorPoolSize poolSizes[] = {
            imagePoolSize,
            camInfoPoolSize};

        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.maxSets = static_cast<uint32_t>(swapchain.imageCount());
        descriptorPoolInfo.poolSizeCount = 2;
        descriptorPoolInfo.pPoolSizes = poolSizes;

        if (vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor pool");
        }

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts(static_cast<uint32_t>(swapchain.imageCount()), descriptorSetLayout);
        VkDescriptorSetAllocateInfo descriptorSetInfo{};
        descriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetInfo.descriptorPool = descriptorPool;
        descriptorSetInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        descriptorSetInfo.pSetLayouts = descriptorSetLayouts.data();

        descriptorSets.resize(descriptorSetLayouts.size());
        if (vkAllocateDescriptorSets(device, &descriptorSetInfo, descriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor sets");
        }
        // =====

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout");
        }

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = shaderStageInfo;
        pipelineInfo.layout = pipelineLayout;

        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create compute pipeline!");
        }

        vkDestroyShaderModule(device, shaderModule, nullptr);
    }

    void createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.computeFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool");
        }
    }

    void createCommandBuffers()
    {
        commandBuffers.resize(swapchain.imageCount());
        camInfoBuffers.resize(swapchain.imageCount());
        camInfoBuffersMemory.resize(swapchain.imageCount());

        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        std::cout << commandBuffers.size() << std::endl;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffer");
        }

        for (size_t i = 0; i < commandBuffers.size(); i++)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageInfo.imageView = swapchain.imageViews[i];

            VkWriteDescriptorSet imageDescriptorWrite{};
            imageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            imageDescriptorWrite.dstSet = descriptorSets[i];
            imageDescriptorWrite.dstBinding = 0;
            imageDescriptorWrite.descriptorCount = 1;
            imageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            imageDescriptorWrite.pImageInfo = &imageInfo;

            VkBufferCreateInfo camInfoBufferInfo{};
            camInfoBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            camInfoBufferInfo.size = sizeof(cameraPosition);
            camInfoBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            camInfoBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(device, &camInfoBufferInfo, nullptr, &camInfoBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create cam info buffer");
            }

            VkMemoryRequirements camInfoMemReq;
            vkGetBufferMemoryRequirements(device, camInfoBuffers[i], &camInfoMemReq);

            VkMemoryAllocateInfo camInfoAllocInfo{};
            camInfoAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            camInfoAllocInfo.allocationSize = camInfoMemReq.size;
            camInfoAllocInfo.memoryTypeIndex = findMemoryType(camInfoMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            if (vkAllocateMemory(device, &camInfoAllocInfo, nullptr, &camInfoBuffersMemory[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to allocate buffer memory");
            }

            vkBindBufferMemory(device, camInfoBuffers[i], camInfoBuffersMemory[i], 0);

            VkDescriptorBufferInfo camInfoBufferDescriptorInfo{};
            camInfoBufferDescriptorInfo.buffer = camInfoBuffers[i];
            camInfoBufferDescriptorInfo.offset = 0;
            camInfoBufferDescriptorInfo.range = VK_WHOLE_SIZE;

            VkWriteDescriptorSet camInfoDescriptorWrite{};
            camInfoDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            camInfoDescriptorWrite.dstSet = descriptorSets[i];
            camInfoDescriptorWrite.dstBinding = 1;
            camInfoDescriptorWrite.descriptorCount = 1;
            camInfoDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            camInfoDescriptorWrite.pBufferInfo = &camInfoBufferDescriptorInfo;

            VkWriteDescriptorSet descriptorWrites[] = {imageDescriptorWrite, camInfoDescriptorWrite};

            vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to begin recording command buffer");
            }

            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

            VkImageSubresourceRange imageRange{};
            imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageRange.baseMipLevel = 0;
            imageRange.levelCount = VK_REMAINING_MIP_LEVELS;
            imageRange.baseArrayLayer = 0;
            imageRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

            VkImageMemoryBarrier preImageBarrier{};
            preImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            preImageBarrier.srcAccessMask = 0;
            preImageBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            preImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            preImageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            preImageBarrier.srcQueueFamilyIndex = queueFamilyIndices.presentFamily.value();
            preImageBarrier.dstQueueFamilyIndex = queueFamilyIndices.computeFamily.value();
            preImageBarrier.image = swapchain.images[i];
            preImageBarrier.subresourceRange = imageRange;

            vkCmdPipelineBarrier(
                commandBuffers[i],
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &preImageBarrier);

            vkCmdDispatch(commandBuffers[i], WIDTH, HEIGHT, 1);

            VkImageMemoryBarrier postImageBarrier{};
            preImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            preImageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            preImageBarrier.dstAccessMask = 0;
            preImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            preImageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            preImageBarrier.srcQueueFamilyIndex = queueFamilyIndices.computeFamily.value();
            preImageBarrier.dstQueueFamilyIndex = queueFamilyIndices.presentFamily.value();
            preImageBarrier.image = swapchain.images[i];
            preImageBarrier.subresourceRange = imageRange;

            vkCmdPipelineBarrier(
                commandBuffers[i],
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &preImageBarrier);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to record command buffer");
            }
        }
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(swapchain.imageCount(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            {

                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void drawFrame()
    {
        vkWaitForFences(device, 1, &inFlightFences[swapchain.currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, imageAvailableSemaphores[swapchain.currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        imagesInFlight[imageIndex] = inFlightFences[swapchain.currentFrame];

        // TODO: "push constants" are faster way to push small buffers to shaders
        void *data;
        vkMapMemory(device, camInfoBuffersMemory[imageIndex], 0, sizeof(cameraPosition), 0, &data);
        memcpy(data, &cameraPosition, sizeof(cameraPosition));
        vkUnmapMemory(device, camInfoBuffersMemory[imageIndex]);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[swapchain.currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        //VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        //submitInfo.signalSemaphoreCount = 1;
        //submitInfo.pSignalSemaphores = signalSemaphores;
        submitInfo.signalSemaphoreCount = 0;

        vkResetFences(device, 1, &inFlightFences[swapchain.currentFrame]);

        if (vkQueueSubmit(computeQueue, 1, &submitInfo, inFlightFences[swapchain.currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        VkSwapchainKHR swapchains[] = {swapchain.swapchain};

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(presentQueue, &presentInfo);

        vkQueueWaitIdle(presentQueue);

        swapchain.currentFrame = (swapchain.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            if (rightArrowDown)
            {
                cameraPosition.x += 0.01;
            }
            if (leftArrowDown)
            {
                cameraPosition.x -= 0.01;
            }
            if (upArrowDown)
            {
                cameraPosition.y += 0.01;
            }
            if (downArrowDown)
            {
                cameraPosition.y -= 0.01;
            }

            drawFrame();

            //std::cout << cameraPosition.x << " " << cameraPosition.y << std::endl;
        }

        vkDeviceWaitIdle(device);
    }

    void cleanup()
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyPipeline(device, computePipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        for (size_t i = 0; i < swapchain.imageCount(); i++)
        {
            vkDestroyBuffer(device, camInfoBuffers[i], nullptr);
            vkFreeMemory(device, camInfoBuffersMemory[i], nullptr);
        }

        swapchain.cleanup(device);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main()
{
    App app;

    try
    {
        app.run();
    }
    catch (const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}