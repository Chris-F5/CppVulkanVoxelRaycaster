#include "renderer.hpp"

#include "buffer.hpp"
#include "shader_module.hpp"
#include "command_buffers.hpp"
#include "exceptions.hpp"

const uint NODE_TYPE_PARENT = 0;
const uint NODE_TYPE_COLORED = 1;
const uint NODE_TYPE_EMPTY = 2;
const uint octree[] = {
    //  Header              | R    G    B    |  Children......................
    NODE_TYPE_PARENT, 000, 000, 000, 12, 24, 36, 48, 60, 72, 84, 96,
    NODE_TYPE_EMPTY, 000, 000, 000, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_COLORED, 000, 255, 000, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_EMPTY, 000, 000, 255, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_COLORED, 255, 255, 000, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_COLORED, 255, 000, 255, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_COLORED, 000, 255, 255, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_PARENT, 255, 255, 255, 108, 120, 132, 144, 156, 168, 180, 192,
    NODE_TYPE_PARENT, 255, 255, 255, 108, 168, 132, 144, 156, 168, 180, 204,
    NODE_TYPE_COLORED, 000, 000, 000, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_EMPTY, 100, 000, 000, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_COLORED, 000, 100, 000, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_COLORED, 000, 000, 100, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_COLORED, 100, 100, 000, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_COLORED, 100, 000, 100, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_EMPTY, 000, 100, 100, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_COLORED, 100, 100, 100, 00, 00, 00, 00, 00, 00, 00, 00,
    NODE_TYPE_PARENT, 000, 000, 000, 108, 120, 132, 144, 156, 168, 180, 192};

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

    VkBufferCreateInfo camInfoBufferCreateInfo{};
    camInfoBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    camInfoBufferCreateInfo.pNext = nullptr;
    camInfoBufferCreateInfo.flags = 0;
    camInfoBufferCreateInfo.size = sizeof(CamInfoBuffer);
    camInfoBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    camInfoBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    std::vector<VkBuffer> camInfoBuffers = createBuffers(device, &camInfoBufferCreateInfo, swapchain.imageCount());
    std::vector<VkDeviceMemory> camInfoBuffersMemory = allocateBuffers(
        device,
        physicalDevice,
        camInfoBuffers,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkBufferCreateInfo octreeBufferCreateInfo{};
    octreeBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    camInfoBufferCreateInfo.pNext = nullptr;
    camInfoBufferCreateInfo.flags = 0;
    octreeBufferCreateInfo.size = sizeof(uint) * 240;
    octreeBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    octreeBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer octreeBuffer = createBuffers(device, &octreeBufferCreateInfo, 1)[0];
    VkDeviceMemory octreeBufferMemory = allocateBuffers(
        device,
        physicalDevice,
        std::vector<VkBuffer>{octreeBuffer},
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)[0];

    // DESCRIPTOR SETS

    DescriptorCreateInfo swapchainImageDescriptor{};
    swapchainImageDescriptor.binding = 0;
    swapchainImageDescriptor.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    swapchainImageDescriptor.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    swapchainImageDescriptor.imageViews = swapchain.imageViews;
    swapchainImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    DescriptorCreateInfo camInfoDescroptor{};
    camInfoDescroptor.binding = 1;
    camInfoDescroptor.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    camInfoDescroptor.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    camInfoDescroptor.buffers = camInfoBuffers;

    DescriptorCreateInfo octreeDescriptor{};
    octreeDescriptor.binding = 2;
    octreeDescriptor.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    octreeDescriptor.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    octreeDescriptor.buffers = std::vector<VkBuffer>(swapchain.imageCount(), octreeBuffer);

    std::vector<DescriptorCreateInfo>
        descriptorInfos{swapchainImageDescriptor, camInfoDescroptor, octreeDescriptor};

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

    VkSemaphore imageAvailableSemaphore = createSemaphore(device);
    VkSemaphore renderFinishSemaphore = createSemaphore(device);

    // OCTREE

    void *data;
    vkMapMemory(device, octreeBufferMemory, 0, sizeof(octree), 0, &data);
    memcpy(data, &octree, sizeof(octree));
    vkUnmapMemory(device, octreeBufferMemory);

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
    renderer.octreeBuffer = octreeBuffer;
    renderer.octreeBufferMemory = octreeBufferMemory;
    renderer.pipeline = pipeline;
    renderer.commandBuffers = commandBuffers;
    renderer.imageAvailableSemaphore = imageAvailableSemaphore;
    renderer.renderFinishSemaphore = renderFinishSemaphore;

    return renderer;
}

void drawFrame(Renderer *renderer, CamInfoBuffer *camInfo)
{
    uint32_t imageIndex;
    vkAcquireNextImageKHR(
        renderer->device,
        renderer->swapchain.swapchain,
        UINT64_MAX,
        renderer->imageAvailableSemaphore,
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
    submitInfo.pWaitSemaphores = &renderer->imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderer->commandBuffers.buffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderer->renderFinishSemaphore;

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
    presentInfo.pWaitSemaphores = &renderer->renderFinishSemaphore;

    vkQueuePresentKHR(renderer->presentQueue, &presentInfo);

    vkDeviceWaitIdle(renderer->device);
}

void cleanupRenderer(Renderer *renderer)
{
    vkDestroySemaphore(renderer->device, renderer->imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(renderer->device, renderer->renderFinishSemaphore, nullptr);

    vkDestroyCommandPool(renderer->device, renderer->commandBuffers.pool, nullptr);

    vkDestroyDescriptorPool(renderer->device, renderer->descriptorSets.pool, nullptr);
    vkDestroyDescriptorSetLayout(renderer->device, renderer->descriptorSets.layout, nullptr);

    vkDestroyBuffer(renderer->device, renderer->octreeBuffer, nullptr);
    vkFreeMemory(renderer->device, renderer->octreeBufferMemory, nullptr);

    for (int i = 0; i < renderer->swapchain.imageCount(); i++)
    {
        vkDestroyBuffer(renderer->device, renderer->camInfoBuffers[i], nullptr);
        vkFreeMemory(renderer->device, renderer->camInfoBuffersMemory[i], nullptr);
    }

    vkDestroyPipeline(renderer->device, renderer->pipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(renderer->device, renderer->pipeline.layout, nullptr);

    for (VkImageView imageView : renderer->swapchain.imageViews)
    {
        vkDestroyImageView(renderer->device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(renderer->device, renderer->swapchain.swapchain, nullptr);
    vkDestroyDevice(renderer->device, nullptr);
    vkDestroySurfaceKHR(renderer->instance, renderer->surface, nullptr);
    vkDestroyInstance(renderer->instance, nullptr);
}
