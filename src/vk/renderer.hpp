#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>

#include "swapchain.hpp"
#include "device.hpp"
#include "descriptor_set.hpp"
#include "pipeline.hpp"
#include "synchronization.hpp"
#include "command_buffers.hpp"

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

    VkBuffer sceneStagingBuffer;
    VkDeviceMemory sceneStagingBufferMemory;

    VkImage sceneImage;
    VkImageView sceneImageView;
    VkDeviceMemory sceneImageMemory;

    VkBuffer paletteStagingBuffer;
    VkDeviceMemory paletteStagingBufferMemory;

    VkImage paletteImage;
    VkImageView paletteImageView;
    VkDeviceMemory paletteImageMemory;

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
void updateScene(Renderer *renderer, unsigned char *palette, unsigned char *voxels);
void drawFrame(Renderer *rendrer, CamInfoBuffer *camInfo);
void cleanupRenderer(Renderer *renderPipeline);