#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>
#include <iostream>

#include "window.hpp"
#include "vk/renderer.hpp"
#include "input.hpp"
#include "camera_controller.hpp"
#include "octree_generator.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

void mainLoop(GLFWwindow *window, Renderer *renderer)
{
    Camera camera;
    camera.position = glm::vec3(0.0);
    camera.degreesRotation = glm::vec3(0.0);
    camera.speed = 0.5;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        InputState inputState = pollInput(window);
        updateCamera(&camera, inputState);

        CamInfoBuffer camInfo;
        camInfo.camPos = glm::vec4(camera.position, 0);
        camInfo.camRotMat = camera.camToWorldRotMat();

        drawFrame(renderer, &camInfo);
    }
}

int main()
{
    std::vector<uint> octree = getOctreeFromFile("scene.ply");
    for (int i = 0; i < octree.size(); i += 5)
    {
        if ((i - 5) % 8 == 0)
        {
            std::cout << "--- " << i << " ---\n";
        }
        std::cout << octree[i] << " " << octree[i + 1] << " " << octree[i + 2] << " " << octree[i + 3] << " " << octree[i + 4] << "\n";
    }

    glfwInit();
    GLFWwindow *window = createWindow("Vulkan Test App", WIDTH, HEIGHT);

    // Depth of octree must be 8
    Renderer renderer = createRenderer(window, enableValidationLayers, octree);

    enableStickyKeys(window);
    mainLoop(window, &renderer);

    vkDeviceWaitIdle(renderer.device);

    cleanupRenderer(&renderer);
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}