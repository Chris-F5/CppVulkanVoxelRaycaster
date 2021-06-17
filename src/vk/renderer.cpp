#include "renderer.hpp"

#include "buffer.hpp"
#include "shader_module.hpp"
#include "command_buffers.hpp"
#include "exceptions.hpp"

Renderer createRenderer(GLFWwindow *window, bool enableValidationLayers)
{
    // DEVICE

    VkInstance instance = createInstance(enableValidationLayers);
    VkSurfaceKHR surface = createSurface(instance, window);
    VkPhysicalDevice physicalDevice = pickPhysicalDevice(instance, surface);
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
    VkDevice device = createLogicalDevice(physicalDevice, enableValidationLayers, queueFamilyIndices);

    VkQueue computeQueue;
    vkGetDeviceQueue(device, queueFamilyIndices.computeFamily.value(), 0, &computeQueue);
    VkQueue presentQueue;
    vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);

    // SWAPCHAIN

    Swapchain swapchain = createSwapchain(device, physicalDevice, window, surface, queueFamilyIndices);

    // BUFFERS

    VkBufferCreateInfo camInfoBufferCreateInfo;
    camInfoBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    camInfoBufferCreateInfo.size = sizeof(CamInfoBuffer);
    camInfoBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    camInfoBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    std::vector<VkBuffer> camInfoBuffers = createBuffers(device, &camInfoBufferCreateInfo, swapchain.imageCount());
    std::vector<VkDeviceMemory> camInfoBuffersMemory = allocateBuffers(
        device,
        physicalDevice,
        camInfoBuffers,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // DESCRIPTOR SETS

    DescriptorCreateInfo camInfoDescroptor{};
    camInfoDescroptor.binding = 1;
    camInfoDescroptor.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    camInfoDescroptor.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    camInfoDescroptor.buffers = camInfoBuffers;

    DescriptorCreateInfo swapchainImageDescriptor{};
    swapchainImageDescriptor.binding = 0;
    swapchainImageDescriptor.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    swapchainImageDescriptor.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    swapchainImageDescriptor.imageViews = swapchain.imageViews;
    swapchainImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    std::vector<DescriptorCreateInfo> descriptorInfos{swapchainImageDescriptor, camInfoDescroptor};

    DescriptorSets descriptorSets = createDescriptorSets(device, descriptorInfos, swapchain.imageCount());

    // PIPELINE

    VkShaderModule renderShader = createShaderModule(device, "shader.spv");

    PipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.computeShader = renderShader;
    pipelineCreateInfo.descriptorSetLayouts =
        std::vector<VkDescriptorSetLayout>{descriptorSets.layout};
    pipelineCreateInfo.computeShaderStageCreateFlags = 0;
    pipelineCreateInfo.pipelineCreateFlags = 0;

    Pipeline pipeline = createPipeline(device, pipelineCreateInfo);

    // COMMAND BUFFERS

    CommandBuffersCreateInfo commandBufferInfo{};
    commandBufferInfo.count = swapchain.imageCount();
    commandBufferInfo.queueFamilies = queueFamilyIndices;
    commandBufferInfo.poolCreateFlags = 0;
    commandBufferInfo.usageFlags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferInfo.pipeline = pipeline;
    commandBufferInfo.descriptorSets =
        std::vector<DescriptorSets>{descriptorSets};
    commandBufferInfo.images = swapchain.images;
    commandBufferInfo.imageExtent = swapchain.extent;

    CommandBuffers commandBuffers = createCommandBuffers(device, commandBufferInfo);

    vkDestroyShaderModule(device, renderShader, nullptr);

    // SYNCHRONIZATION OBJECTS

    SynchronizationObjects synchronizationObjects = createSynchronizationObjects(device);

    // RETURN

    Renderer renderer{};
    renderer.instance = instance;
    renderer.physicalDevice = physicalDevice;
    renderer.device = device;
    renderer.surface = surface;
    renderer.queueFamilyIndices = queueFamilyIndices;
    renderer.computeQueue = computeQueue;
    renderer.presentQueue = presentQueue;
    renderer.swapchain = swapchain;
    renderer.descriptorSets = descriptorSets;
    renderer.camInfoBuffers = camInfoBuffers;
    renderer.camInfoBuffersMemory = camInfoBuffersMemory;
    renderer.pipeline = pipeline;
    renderer.commandBuffers = commandBuffers;
    renderer.synchronizationObjects = synchronizationObjects;

    return renderer;
}

void drawFrame(Renderer *renderer, CamInfoBuffer *camInfo)
{
    uint32_t imageIndex;
    vkAcquireNextImageKHR(
        renderer->device,
        renderer->swapchain.swapchain,
        UINT64_MAX,
        renderer->synchronizationObjects.imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &imageIndex);

    // TODO: "push constants" are faster way to push small buffers to shaders
    void *data;
    vkMapMemory(renderer->device, renderer->camInfoBuffersMemory[imageIndex], 0, sizeof(*camInfo), 0, &data);
    memcpy(data, camInfo, sizeof(*camInfo));
    vkUnmapMemory(renderer->device, renderer->camInfoBuffersMemory[imageIndex]);

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &renderer->synchronizationObjects.imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderer->commandBuffers.buffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderer->synchronizationObjects.renderFinishedSemaphore;

    handleVkResult(
        vkQueueSubmit(renderer->computeQueue, 1, &submitInfo, VK_NULL_HANDLE),
        "submitting compute queue");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSwapchainKHR swapchains[] = {renderer->swapchain.swapchain};

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderer->synchronizationObjects.renderFinishedSemaphore;

    vkQueuePresentKHR(renderer->presentQueue, &presentInfo);
}

void cleanupRenderer(Renderer *renderPipeline)
{
}