#include "renderer.hpp"

#include "buffer.hpp"
#include "shader_module.hpp"
#include "command_buffers.hpp"
#include "exceptions.hpp"

const size_t OCTREE_BUFFER_SIZE = 1000000 * sizeof(uint);

Renderer createRenderer(GLFWwindow *window, bool enableValidationLayers)
{
    Renderer renderer{};
    renderer.currentFrame = 0;

    // DEVICE

    renderer.instance = createInstance(enableValidationLayers);
    renderer.surface = createSurface(renderer.instance, window);
    renderer.physicalDevice = pickPhysicalDevice(renderer.instance, renderer.surface);
    renderer.computeAndPresentQueueFamily = 
        pickComputeAndPresentFamily(renderer.physicalDevice, renderer.surface).index;
    renderer.device = createLogicalDevice(
        renderer.physicalDevice,
        enableValidationLayers,
        1,
        &renderer.computeAndPresentQueueFamily);

    vkGetDeviceQueue(renderer.device, renderer.computeAndPresentQueueFamily, 0, &renderer.computeAndPresentQueue);

    // SWAPCHAIN

    renderer.swapchain = createSwapchain(
        renderer.device,
        renderer.physicalDevice,
        window,
        renderer.surface);

    // BUFFERS

    renderer.camInfoBuffers.resize(renderer.swapchain.imageCount());
    createBuffers(
        renderer.device,
        0,
        sizeof(CamInfoBuffer),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        renderer.camInfoBuffers.size(),
        renderer.camInfoBuffers.data());

    renderer.camInfoBuffersMemory.resize(renderer.swapchain.imageCount());
    allocateBuffers(
        renderer.device,
        renderer.physicalDevice,
        renderer.camInfoBuffers.size(),
        renderer.camInfoBuffers.data(),
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        renderer.camInfoBuffersMemory.data());
    
    createBuffers(
        renderer.device,
        0,
        OCTREE_BUFFER_SIZE,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        1,
        &renderer.octreeStagingBuffer);
    allocateBuffers(
        renderer.device,
        renderer.physicalDevice,
        1,
        &renderer.octreeStagingBuffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &renderer.octreeStagingBufferMemory);

    createBuffers(
        renderer.device,
        0,
        OCTREE_BUFFER_SIZE,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        1,
        &renderer.octreeBuffer);
    allocateBuffers(
        renderer.device,
        renderer.physicalDevice,
        1,
        &renderer.octreeBuffer,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &renderer.octreeBufferMemory);

    // DESCRIPTOR SETS

    DescriptorCreateInfo swapchainImageDescriptor{};
    swapchainImageDescriptor.binding = 0;
    swapchainImageDescriptor.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    swapchainImageDescriptor.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    swapchainImageDescriptor.imageViews = renderer.swapchain.imageViews;
    swapchainImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    DescriptorCreateInfo camInfoDescroptor{};
    camInfoDescroptor.binding = 1;
    camInfoDescroptor.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    camInfoDescroptor.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    camInfoDescroptor.buffers = renderer.camInfoBuffers;

    DescriptorCreateInfo octreeDescriptor{};
    octreeDescriptor.binding = 2;
    octreeDescriptor.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    octreeDescriptor.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    octreeDescriptor.buffers = std::vector<VkBuffer>(renderer.swapchain.imageCount(), renderer.octreeBuffer);

    std::vector<DescriptorCreateInfo>
        descriptorInfos{swapchainImageDescriptor, camInfoDescroptor, octreeDescriptor};

    renderer.descriptorSets = createDescriptorSets(renderer.device, descriptorInfos, renderer.swapchain.imageCount());

    // PIPELINE

    VkShaderModule renderShader = createShaderModule(renderer.device, "shader.spv");

    PipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.computeShader = renderShader;
    pipelineCreateInfo.descriptorSetLayouts =
        std::vector<VkDescriptorSetLayout>{renderer.descriptorSets.layout};
    pipelineCreateInfo.computeShaderStageCreateFlags = 0;
    pipelineCreateInfo.pipelineCreateFlags = 0;

    renderer.pipeline = createPipeline(renderer.device, pipelineCreateInfo);

    vkDestroyShaderModule(renderer.device, renderShader, nullptr);

    // COMMAND BUFFERS

    renderer.computeCommandPool = createCommandPool(
        renderer.device,
        0,
        renderer.computeAndPresentQueueFamily);

    renderer.transientComputeCommandPool = createCommandPool(
        renderer.device,
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        renderer.computeAndPresentQueueFamily);

    renderer.renderCommandBuffers.resize(renderer.swapchain.imageCount());
    allocateCommandBuffers(
        renderer.device,
        renderer.computeCommandPool,
        renderer.swapchain.imageCount(),
        renderer.renderCommandBuffers.data());

    for(int i = 0 ; i < renderer.renderCommandBuffers.size(); i++){
        VkDescriptorSet descriptorSetstToBndToRenderCommand[] = 
            {renderer.descriptorSets.sets[i]};
        recordRenderCommandBuffer(
            renderer.renderCommandBuffers[i],
            renderer.pipeline.layout,
            renderer.pipeline.pipeline,
            sizeof(descriptorSetstToBndToRenderCommand) / sizeof(descriptorSetstToBndToRenderCommand)[0],
            descriptorSetstToBndToRenderCommand,
            renderer.swapchain.images[i],
            renderer.swapchain.extent,
            renderer.computeAndPresentQueueFamily,
            renderer.computeAndPresentQueueFamily);
    }

    // SYNCHRONIZATION OBJECTS
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        renderer.imageAvailableSemaphores[i] = createSemaphore(renderer.device);
        renderer.renderFinishSemaphores[i] = createSemaphore(renderer.device);
        renderer.inFlightFences[i] = createFence(renderer.device, VK_FENCE_CREATE_SIGNALED_BIT);
    }
    renderer.imagesInFlight = std::vector<VkFence>(renderer.swapchain.imageCount(), VK_NULL_HANDLE);

    return renderer;
}

void octreeStagingTransfer(Renderer *renderer, size_t size){
    VkCommandBuffer commandBuffer;
    allocateCommandBuffers(
        renderer->device,
        renderer->transientComputeCommandPool,
        1,
        &commandBuffer);
    recordTransferCommandBuffer(
        commandBuffer,
        renderer->octreeStagingBuffer,
        renderer->octreeBuffer,
        size * sizeof(uint32_t));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;

    vkQueueSubmit(renderer->computeAndPresentQueue, 1, &submitInfo, VK_NULL_HANDLE);

    vkQueueWaitIdle(renderer->computeAndPresentQueue);

    vkFreeCommandBuffers(renderer->device, renderer->transientComputeCommandPool, 1, &commandBuffer);
}

void updateOctree(Renderer *renderer, size_t octreeSize, uint32_t* octree){
    size_t memorySize = octreeSize * sizeof(uint);
    void *data;
    vkMapMemory(renderer->device, renderer->octreeStagingBufferMemory, 0, memorySize , 0, &data);
    memcpy(data, octree, memorySize);
    vkUnmapMemory(renderer->device, renderer->octreeStagingBufferMemory);

    octreeStagingTransfer(renderer, octreeSize);
}

void drawFrame(Renderer *renderer, CamInfoBuffer *camInfo)
{
    vkWaitForFences(renderer->device, 1, &renderer->inFlightFences[renderer->currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(
        renderer->device,
        renderer->swapchain.swapchain,
        UINT64_MAX,
        renderer->imageAvailableSemaphores[renderer->currentFrame],
        VK_NULL_HANDLE,
        &imageIndex);

    if(renderer->imagesInFlight[imageIndex] != VK_NULL_HANDLE){
        vkWaitForFences(renderer->device, 1, &renderer->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    renderer->imagesInFlight[imageIndex] = renderer->inFlightFences[renderer->currentFrame];
    vkResetFences(renderer->device, 1, &renderer->inFlightFences[renderer->currentFrame]);

    // TODO: "push constants" are faster way to push small buffers to shaders
    void *data;
    vkMapMemory(renderer->device, renderer->camInfoBuffersMemory[imageIndex], 0, sizeof(*camInfo), 0, &data);
    memcpy(data, camInfo, sizeof(*camInfo));
    vkUnmapMemory(renderer->device, renderer->camInfoBuffersMemory[imageIndex]);

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &renderer->imageAvailableSemaphores[renderer->currentFrame];
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderer->renderCommandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderer->renderFinishSemaphores[renderer->currentFrame];

    handleVkResult(
        vkQueueSubmit(renderer->computeAndPresentQueue, 1, &submitInfo, renderer->inFlightFences[renderer->currentFrame]),
        "submitting compute queue");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSwapchainKHR swapchains[] = {renderer->swapchain.swapchain};

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderer->renderFinishSemaphores[renderer->currentFrame];

    vkQueuePresentKHR(renderer->computeAndPresentQueue, &presentInfo);

    renderer->currentFrame = (renderer->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void cleanupRenderer(Renderer *renderer)
{
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        vkDestroySemaphore(renderer->device, renderer->imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(renderer->device, renderer->renderFinishSemaphores[i], nullptr);
        vkDestroyFence(renderer->device, renderer->inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(renderer->device, renderer->computeCommandPool, nullptr);
    vkDestroyCommandPool(renderer->device, renderer->transientComputeCommandPool, nullptr);

    vkDestroyDescriptorPool(renderer->device, renderer->descriptorSets.pool, nullptr);
    vkDestroyDescriptorSetLayout(renderer->device, renderer->descriptorSets.layout, nullptr);

    vkDestroyBuffer(renderer->device, renderer->octreeBuffer, nullptr);
    vkFreeMemory(renderer->device, renderer->octreeBufferMemory, nullptr);

    vkDestroyBuffer(renderer->device, renderer->octreeStagingBuffer, nullptr);
    vkFreeMemory(renderer->device, renderer->octreeStagingBufferMemory, nullptr);

    for (int i = 0; i < renderer->swapchain.imageCount(); i++)
    {
        vkDestroyBuffer(renderer->device, renderer->camInfoBuffers[i], nullptr);
        vkFreeMemory(renderer->device, renderer->camInfoBuffersMemory[i], nullptr);
    }

    vkDestroyPipeline(renderer->device, renderer->pipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(renderer->device, renderer->pipeline.layout, nullptr);

    cleanupSwapchain(renderer->device, renderer->swapchain);
    vkDestroyDevice(renderer->device, nullptr);
    vkDestroySurfaceKHR(renderer->instance, renderer->surface, nullptr);
    vkDestroyInstance(renderer->instance, nullptr);
}
