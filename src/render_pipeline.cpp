#include "render_pipeline.hpp"
#include <vector>
#include <cstring>
#include <iostream>
#include <fstream>

const int MAX_FRAMES_IN_FLIGHT = 2;

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device)
{
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

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
    return descriptorSetLayout;
}

VkDescriptorPool createDescriptorPool(VkDevice device, uint32_t descriptorCount)
{
    VkDescriptorPoolSize imagePoolSize{};
    imagePoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imagePoolSize.descriptorCount = descriptorCount;

    VkDescriptorPoolSize camInfoPoolSize{};
    camInfoPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    camInfoPoolSize.descriptorCount = descriptorCount;

    VkDescriptorPoolSize poolSizes[] = {
        imagePoolSize,
        camInfoPoolSize};

    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.maxSets = descriptorCount;
    descriptorPoolInfo.poolSizeCount = 2;
    descriptorPoolInfo.pPoolSizes = poolSizes;

    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool");
    }
    return descriptorPool;
}

std::vector<VkDescriptorSet> allocateDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorCount)
{

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(descriptorCount, descriptorSetLayout);
    VkDescriptorSetAllocateInfo descriptorSetInfo{};
    descriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetInfo.descriptorPool = descriptorPool;
    descriptorSetInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    descriptorSetInfo.pSetLayouts = descriptorSetLayouts.data();

    std::vector<VkDescriptorSet> descriptorSets(descriptorCount);
    if (vkAllocateDescriptorSets(device, &descriptorSetInfo, descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor sets");
    }
    return descriptorSets;
}

std::vector<VkBuffer> createUniformBuffers(VkDevice device, size_t count)
{
    std::vector<VkBuffer> buffers(count);

    VkBufferCreateInfo camInfoBufferInfo{};
    camInfoBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    camInfoBufferInfo.size = sizeof(UniformData);
    camInfoBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    camInfoBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    for (size_t i = 0; i < count; i++)
    {
        if (vkCreateBuffer(device, &camInfoBufferInfo, nullptr, &buffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create cam info buffer.");
        }
    }
    return buffers;
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

    throw std::runtime_error("Failed to find suitable memory type.");
}

std::vector<VkDeviceMemory> allocateAndBindUniformBuffersMemory(VkDevice device, VkPhysicalDevice physicalDevice, std::vector<VkBuffer> buffers)
{
    std::vector<VkDeviceMemory> buffersMemory(buffers.size());
    for (size_t i = 0; i < buffers.size(); i++)
    {
        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, buffers[i], &memReq);

        VkMemoryAllocateInfo camInfoAllocInfo{};
        camInfoAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        camInfoAllocInfo.allocationSize = memReq.size;
        camInfoAllocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device, &camInfoAllocInfo, nullptr, &buffersMemory[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate buffer memory.");
        }

        vkBindBufferMemory(device, buffers[i], buffersMemory[i], 0);
    }
    return buffersMemory;
}

void initDescriptorSets(VkDevice device, std::vector<VkDescriptorSet> descriptorSets, std::vector<VkImageView> imageViews, std::vector<VkBuffer> uniformBuffers)
{
    size_t count = descriptorSets.size();
    if (imageViews.size() != count || uniformBuffers.size() != count)
    {
        throw std::runtime_error("Cant init descriptor sets because sizes dont match");
    }

    for (size_t i = 0; i < count; i++)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = imageViews[i];

        VkWriteDescriptorSet imageDescriptorWrite{};
        imageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        imageDescriptorWrite.dstSet = descriptorSets[i];
        imageDescriptorWrite.dstBinding = 0;
        imageDescriptorWrite.descriptorCount = 1;
        imageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        imageDescriptorWrite.pImageInfo = &imageInfo;

        VkDescriptorBufferInfo camInfoBufferDescriptorInfo{};
        camInfoBufferDescriptorInfo.buffer = uniformBuffers[i];
        camInfoBufferDescriptorInfo.offset = 0;
        camInfoBufferDescriptorInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet uniformDescriptorWrite{};
        uniformDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformDescriptorWrite.dstSet = descriptorSets[i];
        uniformDescriptorWrite.dstBinding = 1;
        uniformDescriptorWrite.descriptorCount = 1;
        uniformDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformDescriptorWrite.pBufferInfo = &camInfoBufferDescriptorInfo;

        VkWriteDescriptorSet descriptorWrites[] = {imageDescriptorWrite, uniformDescriptorWrite};

        vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
    }
}

std::vector<char> readFile(const std::string &filename)
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

VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &spv)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spv.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(spv.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module.");
    }
    return shaderModule;
}

VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout");
    }
    return pipelineLayout;
}

VkPipeline createPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkShaderModule renderShader)
{
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = renderShader;
    shaderStageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = shaderStageInfo;
    pipelineInfo.layout = pipelineLayout;

    VkPipeline pipeline;
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create compute pipeline.");
    }
    return pipeline;
}

VkCommandPool createCommandPool(VkDevice device, uint32_t queueFamily)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamily;

    VkCommandPool commandPool;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool.");
    }
    return commandPool;
}

std::vector<VkCommandBuffer> createCommandBuffers(
    VkDevice device,
    VkCommandPool commandPool,
    VkPipeline pipeline,
    VkPipelineLayout pipelineLayout,
    std::vector<VkDescriptorSet> descriptorSets,
    std::vector<VkImage> images,
    VkExtent2D imageExtent,
    QueueFamilyIndices queueFamilyIndices)
{
    size_t count = descriptorSets.size();

    std::vector<VkCommandBuffer> commandBuffers(count);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = count;

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffer");
    }

    for (size_t i = 0; i < count; i++)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
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
        preImageBarrier.image = images[i];
        preImageBarrier.subresourceRange = imageRange;

        vkCmdPipelineBarrier(
            commandBuffers[i],
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &preImageBarrier);

        vkCmdDispatch(commandBuffers[i], imageExtent.width, imageExtent.height, 1);

        VkImageMemoryBarrier postImageBarrier{};
        preImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        preImageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        preImageBarrier.dstAccessMask = 0;
        preImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        preImageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        preImageBarrier.srcQueueFamilyIndex = queueFamilyIndices.computeFamily.value();
        preImageBarrier.dstQueueFamilyIndex = queueFamilyIndices.presentFamily.value();
        preImageBarrier.image = images[i];
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
    return commandBuffers;
}

std::vector<VkSemaphore> createSemaphores(VkDevice device, size_t count)
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    std::vector<VkSemaphore> semaphores(count);

    for (size_t i = 0; i < count; i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphores[i]) != VK_SUCCESS)
        {

            throw std::runtime_error("Failed to create semaphore.");
        }
    }
    return semaphores;
}

std::vector<VkFence> createFences(VkDevice device, size_t count)
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    std::vector<VkFence> fences(count);
    for (size_t i = 0; i < count; i++)
    {
        if (vkCreateFence(device, &fenceInfo, nullptr, &fences[i]) != VK_SUCCESS)
        {

            throw std::runtime_error("Failed to create fence.");
        }
    }
    return fences;
}

RenderPipeline createRenderPipeline(GLFWwindow *window, bool enableValidationLayers, std::string shaderFileName)
{
    VkInstance instance = createInstance(enableValidationLayers);
    VkSurfaceKHR surface = createSurface(instance, window);
    VkPhysicalDevice physicalDevice = pickPhysicalDevice(instance, surface);
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
    VkDevice device = createLogicalDevice(physicalDevice, enableValidationLayers, queueFamilyIndices);

    VkQueue computeQueue;
    vkGetDeviceQueue(device, queueFamilyIndices.computeFamily.value(), 0, &computeQueue);
    VkQueue presentQueue;
    vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);

    Swapchain swapchain = createSwapchain(device, physicalDevice, window, surface, queueFamilyIndices);

    std::vector<VkSemaphore> imageAvailableSemaphores = createSemaphores(device, MAX_FRAMES_IN_FLIGHT);
    std::vector<VkSemaphore> renderFinishedSemaphores = createSemaphores(device, MAX_FRAMES_IN_FLIGHT);
    std::vector<VkFence> inFlightFences = createFences(device, MAX_FRAMES_IN_FLIGHT);
    std::vector<VkFence> imageFences(swapchain.imageCount(), VK_NULL_HANDLE);

    VkDescriptorSetLayout descriptorSetLayout = createDescriptorSetLayout(device);
    VkDescriptorPool descriptorPool = createDescriptorPool(device, swapchain.imageCount());
    std::vector<VkDescriptorSet> descriptorSets = allocateDescriptorSets(device, descriptorPool, descriptorSetLayout, swapchain.imageCount());
    std::vector<VkBuffer> uniformBuffers = createUniformBuffers(device, swapchain.imageCount());
    std::vector<VkDeviceMemory> uniformBuffersMemory = allocateAndBindUniformBuffersMemory(device, physicalDevice, uniformBuffers);
    initDescriptorSets(device, descriptorSets, swapchain.imageViews, uniformBuffers);

    std::vector<char> shaderBinary = readFile(shaderFileName);
    VkShaderModule renderShader = createShaderModule(device, shaderBinary);

    VkPipelineLayout pipelineLayout = createPipelineLayout(device, descriptorSetLayout);
    VkPipeline pipeline = createPipeline(device, pipelineLayout, renderShader);

    vkDestroyShaderModule(device, renderShader, nullptr);

    VkCommandPool commandPool = createCommandPool(device, queueFamilyIndices.computeFamily.value());
    std::vector<VkCommandBuffer> commandBuffers = createCommandBuffers(
        device,
        commandPool,
        pipeline,
        pipelineLayout,
        descriptorSets,
        swapchain.images,
        swapchain.extent,
        queueFamilyIndices);

    RenderPipeline renderPipeline = {
        instance,
        physicalDevice,
        device,
        surface,
        queueFamilyIndices,
        computeQueue,
        presentQueue,
        swapchain,
        imageAvailableSemaphores,
        renderFinishedSemaphores,
        inFlightFences,
        imageFences,
        descriptorPool,
        descriptorSetLayout,
        descriptorSets,
        uniformBuffers,
        uniformBuffersMemory,
        pipelineLayout,
        pipeline,
        commandPool,
        commandBuffers};
    return renderPipeline;
}

void drawFrame(RenderPipeline *pipeline, UniformData uniformData)
{
    vkWaitForFences(pipeline->device, 1, &pipeline->inFlightFences[pipeline->swapchain.currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(
        pipeline->device,
        pipeline->swapchain.swapchain,
        UINT64_MAX,
        pipeline->imageAvailableSemaphores[pipeline->swapchain.currentFrame],
        VK_NULL_HANDLE,
        &imageIndex);

    if (pipeline->imageFences[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(pipeline->device, 1, &pipeline->imageFences[imageIndex], VK_TRUE, UINT64_MAX);
    }
    pipeline->imageFences[imageIndex] = pipeline->imageFences[pipeline->swapchain.currentFrame];

    // TODO: "push constants" are faster way to push small buffers to shaders
    void *data;
    vkMapMemory(pipeline->device, pipeline->uniformBuffersMemory[imageIndex], 0, sizeof(uniformData), 0, &data);
    memcpy(data, &uniformData, sizeof(uniformData));
    vkUnmapMemory(pipeline->device, pipeline->uniformBuffersMemory[imageIndex]);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &pipeline->imageAvailableSemaphores[pipeline->swapchain.currentFrame];
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &pipeline->commandBuffers[imageIndex];

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &pipeline->renderFinishedSemaphores[pipeline->swapchain.currentFrame];

    vkResetFences(pipeline->device, 1, &pipeline->inFlightFences[pipeline->swapchain.currentFrame]);

    if (vkQueueSubmit(pipeline->computeQueue, 1, &submitInfo, pipeline->inFlightFences[pipeline->swapchain.currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSwapchainKHR swapchains[] = {pipeline->swapchain.swapchain};

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &pipeline->renderFinishedSemaphores[pipeline->swapchain.currentFrame];

    vkQueuePresentKHR(pipeline->presentQueue, &presentInfo);

    pipeline->swapchain.currentFrame = (pipeline->swapchain.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void cleanupRenderPipeline(RenderPipeline *pipeline)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(pipeline->device, pipeline->renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(pipeline->device, pipeline->imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(pipeline->device, pipeline->inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(pipeline->device, pipeline->commandPool, nullptr);

    vkDestroyPipeline(pipeline->device, pipeline->pipeline, nullptr);
    vkDestroyPipelineLayout(pipeline->device, pipeline->pipelineLayout, nullptr);

    for (size_t i = 0; i < pipeline->swapchain.imageCount(); i++)
    {
        vkDestroyBuffer(pipeline->device, pipeline->uniformBuffers[i], nullptr);
        vkFreeMemory(pipeline->device, pipeline->uniformBuffersMemory[i], nullptr);
    }

    pipeline->swapchain.cleanup(pipeline->device);

    vkDestroyDescriptorSetLayout(pipeline->device, pipeline->descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(pipeline->device, pipeline->descriptorPool, nullptr);
    vkDestroyDevice(pipeline->device, nullptr);
    vkDestroySurfaceKHR(pipeline->instance, pipeline->surface, nullptr);
    vkDestroyInstance(pipeline->instance, nullptr);
}