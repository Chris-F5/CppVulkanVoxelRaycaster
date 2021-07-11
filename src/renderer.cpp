#include "renderer.hpp"

#include "vk/buffer.hpp"
#include "vk/image.hpp"
#include "vk/shader_module.hpp"
#include "vk/command_buffers.hpp"
#include "vk/exceptions.hpp"

const uint32_t MAX_VOX_BLOCK_COUNT = 200;
const uint32_t OBJECT_INFO_MEM_SIZE = 1600;

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

void updatePalette(Renderer *renderer, Palette *palette){
    void *data;
    vkMapMemory(renderer->device, renderer->paletteStagingBufferMemory, 0, 256 * 4, 0, &data);
    memcpy(data, palette, 256 * 4);
    vkUnmapMemory(renderer->device, renderer->paletteStagingBufferMemory);

    VkImageSubresourceRange paletteSubresourceRange = createImageSubresourceRange(
        VK_IMAGE_ASPECT_COLOR_BIT,
        0, 1,
        0, 1
    );

    transitionImageLayout(
        renderer->device,
        renderer->computeAndPresentQueue,
        renderer->transientComputeCommandPool,
        renderer->paletteImage,
        paletteSubresourceRange,
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
        VkOffset3D{0, 0, 0},
        VkExtent3D{256, 1, 1},
        VK_IMAGE_ASPECT_COLOR_BIT,
        0,
        0, 1,
        renderer->paletteStagingBuffer,
        renderer->paletteImage
    );

    transitionImageLayout(
        renderer->device,
        renderer->computeAndPresentQueue,
        renderer->transientComputeCommandPool,
        renderer->paletteImage,
        paletteSubresourceRange,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
    );
}

void updateBlock(Renderer *renderer, int32_t blockIndex, VoxBlock *block){
    void *data;
    vkMapMemory(renderer->device, renderer->voxBlockStagingBufferMemory, 0, sizeof(VoxBlock), 0, &data);
    memcpy(data, block->voxels, sizeof(VoxBlock));
    vkUnmapMemory(renderer->device, renderer->voxBlockStagingBufferMemory);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = blockIndex * sizeof(VoxBlock);
    copyRegion.size = sizeof(VoxBlock);

    bufferTransfer(
        renderer->device,
        renderer->computeAndPresentQueue,
        renderer->transientComputeCommandPool,
        1,
        &copyRegion,
        renderer->voxBlockStagingBuffer,
        renderer->voxBlocksBuffer
    );
}

void updateObject(Renderer *renderer, VoxObject object){
    uint32_t *data;
    vkMapMemory(renderer->device, renderer->objectInfoBufferMemory, 0, OBJECT_INFO_MEM_SIZE, 0, (void**)&data);
    memcpy(&data[0], (void*)&object.paletteIndex, sizeof(uint32_t));
    memcpy(&data[1], (void*)&object.blockWidth, sizeof(uint32_t));
    memcpy(&data[2], (void*)&object.blockHeight, sizeof(uint32_t));
    memcpy(&data[3], (void*)&object.blockDepth, sizeof(uint32_t));
    memcpy(&data[4], (void*)object.blockIndices, OBJECT_INFO_MEM_SIZE - sizeof(uint32_t) * 4);
    vkUnmapMemory(renderer->device, renderer->objectInfoBufferMemory);
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

    // COMMAND POOLS

    renderer.computeCommandPool = createCommandPool(
        renderer.device,
        0,
        renderer.computeAndPresentQueueFamily);

    renderer.transientComputeCommandPool = createCommandPool(
        renderer.device,
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        renderer.computeAndPresentQueueFamily);

    // STAGING BUFFERS

    createBuffer(
        renderer.device,
        renderer.physicalDevice,
        sizeof(VoxBlock),
        0,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &renderer.voxBlockStagingBuffer,
        &renderer.voxBlockStagingBufferMemory
    );

    createBuffer(
        renderer.device,
        renderer.physicalDevice,
        sizeof(Palette),
        0,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &renderer.paletteStagingBuffer,
        &renderer.paletteStagingBufferMemory
    );

    // CAM INFO BUFFERS

    renderer.camInfoBuffers.resize(renderer.swapchain.imageCount());
    renderer.camInfoBuffersMemory.resize(renderer.swapchain.imageCount());
    for (int i = 0 ; i < renderer.swapchain.imageCount(); i++)
        createBuffer(
            renderer.device,
            renderer.physicalDevice,
            sizeof(CamInfoBuffer),
            0,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &renderer.camInfoBuffers[i],
            &renderer.camInfoBuffersMemory[i]
        );

    // OBJECT BUFFER

    createBuffer(
        renderer.device,
        renderer.physicalDevice,
        OBJECT_INFO_MEM_SIZE,
        0,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &renderer.objectInfoBuffer,
        &renderer.objectInfoBufferMemory
    );
    
    // VOX BLOCKS BUFFER

    createBuffer(
        renderer.device,
        renderer.physicalDevice,
        MAX_VOX_BLOCK_COUNT * sizeof(VoxBlock),
        0,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &renderer.voxBlocksBuffer,
        &renderer.voxBlocksBufferMemory
    );

    // PALETTE IMAGE

    createImage(
        renderer.device,
        renderer.physicalDevice,
        VK_IMAGE_TYPE_1D,
        VK_FORMAT_R8G8B8A8_UNORM,
        VkExtent3D{256, 1, 1},
        0,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        1,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        false,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &renderer.paletteImage,
        &renderer.paletteImageMemory
    );

    renderer.paletteImageView = createImageView(
        renderer.device,
        renderer.paletteImage,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_VIEW_TYPE_1D,
        createImageSubresourceRange(
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 1,
            0, 1
        )
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

    DescriptorCreateInfo voxBlocksDescriptor{};
    voxBlocksDescriptor.binding = 2;
    voxBlocksDescriptor.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    voxBlocksDescriptor.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    voxBlocksDescriptor.buffers = std::vector<VkBuffer>(renderer.swapchain.imageCount(), renderer.voxBlocksBuffer);

    DescriptorCreateInfo paletteDescriptor{};
    paletteDescriptor.binding = 3;
    paletteDescriptor.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    paletteDescriptor.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    paletteDescriptor.imageViews = std::vector<VkImageView>(renderer.swapchain.imageCount(), renderer.paletteImageView);
    paletteDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    DescriptorCreateInfo objectInfoDescriptor{};
    objectInfoDescriptor.binding = 4;
    objectInfoDescriptor.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    objectInfoDescriptor.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    objectInfoDescriptor.buffers = std::vector<VkBuffer>(renderer.swapchain.imageCount(), renderer.objectInfoBuffer);

    std::vector<DescriptorCreateInfo>
        descriptorInfos{swapchainImageDescriptor, camInfoDescroptor, voxBlocksDescriptor, paletteDescriptor, objectInfoDescriptor};

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

    vkDestroyBuffer(renderer->device, renderer->voxBlockStagingBuffer, nullptr);
    vkFreeMemory(renderer->device, renderer->voxBlockStagingBufferMemory, nullptr);

    vkDestroyBuffer(renderer->device, renderer->voxBlocksBuffer, nullptr);
    vkFreeMemory(renderer->device, renderer->voxBlocksBufferMemory, nullptr);

    vkDestroyBuffer(renderer->device, renderer->objectInfoBuffer, nullptr);
    vkFreeMemory(renderer->device, renderer->objectInfoBufferMemory, nullptr);

    vkDestroyBuffer(renderer->device, renderer->paletteStagingBuffer, nullptr);
    vkFreeMemory(renderer->device, renderer->paletteStagingBufferMemory, nullptr);

    vkDestroyImageView(renderer->device, renderer->paletteImageView, nullptr);
    vkDestroyImage(renderer->device, renderer->paletteImage, nullptr);
    vkFreeMemory(renderer->device, renderer->paletteImageMemory, nullptr);

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
