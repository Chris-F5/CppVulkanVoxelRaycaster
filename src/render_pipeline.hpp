#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>

#include "swapchain.hpp"
#include "vulkan_device.hpp"

struct UniformData
{
    glm::vec4 camPos;
    glm::mat4 camRotMat;
};

struct RenderPipeline
{
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    QueueFamilyIndices queueFamilyIndices;
    VkQueue computeQueue;
    VkQueue presentQueue;

    Swapchain swapchain;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imageFences;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
};

RenderPipeline createRenderPipeline(GLFWwindow *window, bool enableValidationLayers, std::string shaderFileName);
void drawFrame(RenderPipeline *pipeline, UniformData uniformData);
void cleanupRenderPipeline(RenderPipeline *renderPipeline);