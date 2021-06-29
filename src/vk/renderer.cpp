#include "renderer.hpp"

#include "buffer.hpp"
#include "shader_module.hpp"
#include "command_buffers.hpp"
#include "exceptions.hpp"

const uint SCENE_WIDTH = 3;
const uint SCENE_HEIGHT = 3;
const uint SCENE_DEPTH = 3;

void createRenderCommandBuffers(
    VkDevice device,
    VkCommandPool commandPool,
    uint32_t count,
    VkPipelineLayout pipelineLayout,
    VkPipeline pipeline,
    VkDescriptorSet *descriptorSets,
    VkExtent2D swapchainImageExtent,
    VkImage *swapchainImages,
    uint32_t computeFamilyIndex,
    uint32_t presentFamilyIndex,
    VkCommandBuffer *commandBuffers)
{
    allocateCommandBuffers(device, commandPool, count, commandBuffers);

    for(int i = 0; i < count; i++){
        beginRecordingCommandBuffer(commandBuffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        vkCmdBindDescriptorSets(
            commandBuffers[i],
            VK_PIPELINE_BIND_POINT_COMPUTE,
            pipelineLayout,
            0,
            1,
            &descriptorSets[i],
            0,
            nullptr);

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
        preImageBarrier.srcQueueFamilyIndex = presentFamilyIndex;
        preImageBarrier.dstQueueFamilyIndex = computeFamilyIndex;
        preImageBarrier.image = swapchainImages[i];
        preImageBarrier.subresourceRange = imageRange;

        vkCmdPipelineBarrier(
            commandBuffers[i],
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &preImageBarrier);
        
        vkCmdDispatch(commandBuffers[i], swapchainImageExtent.width, swapchainImageExtent.width, 1);

        VkImageMemoryBarrier postImageBarrier{};
        postImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        postImageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        postImageBarrier.dstAccessMask = 0;
        postImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        postImageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        postImageBarrier.srcQueueFamilyIndex = computeFamilyIndex;
        postImageBarrier.dstQueueFamilyIndex = presentFamilyIndex;
        postImageBarrier.image = swapchainImages[i];
        postImageBarrier.subresourceRange = imageRange;

        vkCmdPipelineBarrier(
            commandBuffers[i],
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &postImageBarrier);

        handleVkResult(
            vkEndCommandBuffer(commandBuffers[i]),
            "ending commanbuffersd buffer recording");
    }
}

void copyBufferToImage(
    VkDevice device,
    VkQueue queue,
    VkCommandPool commandPool,
    VkExtent3D imageExtent,
    VkBuffer buffer,
    VkImage image
){
    VkCommandBuffer commandBuffer;
    allocateCommandBuffers(device, commandPool, 1, &commandBuffer);

    beginRecordingCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkImageSubresourceLayers imageSubresource{};
    imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresource.mipLevel = 0;
    imageSubresource.baseArrayLayer = 0;
    imageSubresource.layerCount = 1;

    VkBufferImageCopy bufferImageCopy{};
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;
    bufferImageCopy.imageSubresource = imageSubresource;
    bufferImageCopy.imageOffset = VkOffset3D{0,0,0};
    bufferImageCopy.imageExtent = imageExtent;

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);

    vkEndCommandBuffer(commandBuffer);

    submitCommandBuffers(
        queue,
        1,
        &commandBuffer,
        0, nullptr, nullptr,
        0, nullptr, 
        VK_NULL_HANDLE
    );

    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void transitionImageLayout(
    VkDevice device,
    VkQueue queue,
    VkCommandPool commandPool,
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkPipelineStageFlags srcStageMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags dstStageMask)
{
    VkCommandBuffer commandBuffer;
    allocateCommandBuffers(device, commandPool, 1, &commandBuffer);

    beginRecordingCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = subresourceRange;

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStageMask, dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    vkEndCommandBuffer(commandBuffer);

    submitCommandBuffers(
        queue,
        1,
        &commandBuffer,
        0, nullptr, nullptr,
        0, nullptr, 
        VK_NULL_HANDLE
    );

    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void updateScene(Renderer *renderer){
    unsigned char voxels[] = {
        255, 1, 2, 3, 4, 5, 6, 7, 8,
        0, 1, 2, 3, 4, 5, 6, 7, 8,
        0, 1, 2, 3, 4, 5, 6, 7, 8
    };
    void *data;
    vkMapMemory(renderer->device, renderer->sceneStagingBufferMemory, 0, sizeof(voxels) , 0, &data);
    memcpy(data, voxels, sizeof(voxels));
    vkUnmapMemory(renderer->device, renderer->sceneStagingBufferMemory);

    transitionImageLayout(
        renderer->device,
        renderer->computeAndPresentQueue,
        renderer->transientComputeCommandPool,
        renderer->sceneImage,
        VK_FORMAT_R8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        0,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT
    );

    copyBufferToImage(
        renderer->device,
        renderer->computeAndPresentQueue,
        renderer->transientComputeCommandPool,
        VkExtent3D{SCENE_WIDTH, SCENE_HEIGHT, SCENE_DEPTH},
        renderer->sceneStagingBuffer,
        renderer->sceneImage
    );

    transitionImageLayout(
        renderer->device,
        renderer->computeAndPresentQueue,
        renderer->transientComputeCommandPool,
        renderer->sceneImage,
        VK_FORMAT_R8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
    );
}

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
        &renderer.computeAndPresentQueueFamily
    );

    vkGetDeviceQueue(renderer.device, renderer.computeAndPresentQueueFamily, 0, &renderer.computeAndPresentQueue);

    // SWAPCHAIN

    renderer.swapchain = createSwapchain(
        renderer.device,
        renderer.physicalDevice,
        window,
        renderer.surface);

    // BUFFERS

    renderer.sceneStagingBuffer = createBuffer(
        renderer.device,
        0,
        SCENE_WIDTH * SCENE_HEIGHT * SCENE_DEPTH * sizeof(unsigned char),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    );

    renderer.sceneStagingBufferMemory = allocateBuffer(
        renderer.device,
        renderer.physicalDevice,
        renderer.sceneStagingBuffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    renderer.camInfoBuffers.resize(renderer.swapchain.imageCount());
    renderer.camInfoBuffersMemory.resize(renderer.swapchain.imageCount());
    for (int i = 0 ; i < renderer.swapchain.imageCount(); i++){
        renderer.camInfoBuffers[i] = createBuffer(
            renderer.device,
            0,
            sizeof(CamInfoBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        );
        renderer.camInfoBuffersMemory[i] = allocateBuffer(
            renderer.device,
            renderer.physicalDevice,
            renderer.camInfoBuffers[i],
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
    }

    // COMMAND POOLS

    renderer.computeCommandPool = createCommandPool(
        renderer.device,
        0,
        renderer.computeAndPresentQueueFamily);

    renderer.transientComputeCommandPool = createCommandPool(
        renderer.device,
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        renderer.computeAndPresentQueueFamily);


    // VOXEL IMAGES

    VkExtent3D sceneExtent{};
    sceneExtent.width = SCENE_WIDTH;
    sceneExtent.height = SCENE_HEIGHT;
    sceneExtent.depth = SCENE_DEPTH;

    renderer.sceneImage = createImage(
        renderer.device,
        0,
        VK_IMAGE_TYPE_3D,
        VK_FORMAT_R8_SRGB,
        sceneExtent,
        1,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED
    );

    renderer.sceneImageMemory = allocateImage(
        renderer.device,
        renderer.physicalDevice,
        renderer.sceneImage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    updateScene(&renderer);

    renderer.sceneImageView = createImageView(
        renderer.device,
        renderer.sceneImage,
        VK_FORMAT_R8_SRGB,
        VK_IMAGE_VIEW_TYPE_3D
    );

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

    DescriptorCreateInfo sceneDescriptor{};
    sceneDescriptor.binding = 2;
    sceneDescriptor.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sceneDescriptor.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    sceneDescriptor.imageViews = std::vector<VkImageView>(renderer.swapchain.imageCount(), renderer.sceneImageView);
    sceneDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::vector<DescriptorCreateInfo>
        descriptorInfos{swapchainImageDescriptor, camInfoDescroptor, sceneDescriptor};

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

    renderer.renderCommandBuffers.resize(renderer.swapchain.imageCount());
    createRenderCommandBuffers(
        renderer.device,
        renderer.computeCommandPool,
        renderer.swapchain.imageCount(),
        renderer.pipeline.layout,
        renderer.pipeline.pipeline,
        renderer.descriptorSets.sets.data(),
        renderer.swapchain.extent,
        renderer.swapchain.images.data(),
        renderer.computeAndPresentQueueFamily,
        renderer.computeAndPresentQueueFamily,
        renderer.renderCommandBuffers.data()
    );

    // SYNCHRONIZATION OBJECTS

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        renderer.imageAvailableSemaphores[i] = createSemaphore(renderer.device);
        renderer.renderFinishSemaphores[i] = createSemaphore(renderer.device);
        renderer.inFlightFences[i] = createFence(renderer.device, VK_FENCE_CREATE_SIGNALED_BIT);
    }
    renderer.imagesInFlight = std::vector<VkFence>(renderer.swapchain.imageCount(), VK_NULL_HANDLE);

    return renderer;
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

    submitCommandBuffers(
        renderer->computeAndPresentQueue,
         1, &renderer->renderCommandBuffers[imageIndex],
         1, &renderer->imageAvailableSemaphores[renderer->currentFrame], waitStages,
         1, &renderer->renderFinishSemaphores[renderer->currentFrame],
         renderer->inFlightFences[renderer->currentFrame]);

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

    vkDestroyImageView(renderer->device, renderer->sceneImageView, nullptr);
    vkDestroyImage(renderer->device, renderer->sceneImage, nullptr);
    vkFreeMemory(renderer->device, renderer->sceneImageMemory, nullptr);

    vkDestroyBuffer(renderer->device, renderer->sceneStagingBuffer, nullptr);
    vkFreeMemory(renderer->device, renderer->sceneStagingBufferMemory, nullptr);

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
