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
    QueueFamilyIndices queueFamilyIndices;
    VkQueue computeQueue;
    VkQueue presentQueue;

    Swapchain swapchain;

    DescriptorSets descriptorSets;

    std::vector<VkBuffer> camInfoBuffers;
    std::vector<VkDeviceMemory> camInfoBuffersMemory;

    VkBuffer octreeBuffer;
    VkDeviceMemory octreeBufferMemory;

    Pipeline pipeline;

    CommandBuffers commandBuffers;

    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore renderFinishSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
    std::vector<VkFence> imagesInFlight;

    uint32_t currentFrame;
};

Renderer createRenderer(GLFWwindow *window, bool enableValidationLayers);
void updateOctree(Renderer *renderer, uint* octree, size_t octreeSize);
void drawFrame(Renderer *rendrer, CamInfoBuffer *camInfo);
void cleanupRenderer(Renderer *renderPipeline);