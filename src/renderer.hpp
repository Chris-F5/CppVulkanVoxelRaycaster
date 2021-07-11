#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>

#include "vk/swapchain.hpp"
#include "vk/device.hpp"
#include "vk/descriptor_set.hpp"
#include "vk/pipeline.hpp"
#include "vk/synchronization.hpp"
#include "vk/command_buffers.hpp"
#include "vox_object.hpp"

const size_t MAX_FRAMES_IN_FLIGHT = 3;

struct CamInfoBuffer
{
    glm::vec4 camPos;
    glm::mat4 camRotMat;
};

struct Renderer
{
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    uint32_t computeAndPresentQueueFamily;
    VkQueue computeAndPresentQueue;

    Swapchain swapchain;

    DescriptorSets descriptorSets;

    std::vector<VkBuffer> camInfoBuffers;
    std::vector<VkDeviceMemory> camInfoBuffersMemory;

    VkBuffer voxBlockStagingBuffer;
    VkDeviceMemory voxBlockStagingBufferMemory;

    VkBuffer voxBlocksBuffer;
    VkDeviceMemory voxBlocksBufferMemory;

    VkBuffer paletteStagingBuffer;
    VkDeviceMemory paletteStagingBufferMemory;

    VkImage paletteImage;
    VkDeviceMemory paletteImageMemory;
    VkImageView paletteImageView;

    VkBuffer objectInfoBuffer;
    VkDeviceMemory objectInfoBufferMemory;

    Pipeline pipeline;

    VkCommandPool computeCommandPool;
    VkCommandPool transientComputeCommandPool;

    std::vector<VkCommandBuffer> renderCommandBuffers;

    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore renderFinishSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
    std::vector<VkFence> imagesInFlight;

    uint32_t currentFrame;
};

Renderer createRenderer(GLFWwindow *window, bool enableValidationLayers);

void updateObject(Renderer *renderer, VoxObject object);
void updateBlock(Renderer *renderer, int32_t blockIndex, VoxBlock *block);
void updatePalette(Renderer *renderer, Palette *palette);

void drawFrame(Renderer *rendrer, CamInfoBuffer *camInfo);
void cleanupRenderer(Renderer *renderPipeline);