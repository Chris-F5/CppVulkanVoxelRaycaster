#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>
#include <iostream>

#include "window.hpp"
#include "renderer.hpp"
#include "input.hpp"
#include "camera_controller.hpp"
#include "vox_object.hpp"

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
    camera.position = glm::vec3(107.9, 52.4, 89.7);
    camera.degreesRotation = glm::vec3(-125, -22, 0);
    camera.speed = 30;
    camera.rotateSpeed = 70;

    double thisSecondStartTime = glfwGetTime();
    double previousFrameTime = 0;
    uint framesThisSecond = 0;

    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        framesThisSecond++;
        float deltaTime = currentTime - previousFrameTime;
        if(currentTime - thisSecondStartTime >= 1.0){
            std::string fps = std::to_string(framesThisSecond);
            std::string title = "Ray Caster fps: " + fps;
            glfwSetWindowTitle(window, title.c_str());

            framesThisSecond = 0;
            thisSecondStartTime = currentTime;
        }

        glfwPollEvents();
        InputState inputState = pollInput(window);
        updateCamera(&camera, inputState, deltaTime);
        if (inputState.p){
            printf(
                "cam info:\n\tpos: %.1f %.1f %.1f\n\trot: %.0f %.0f %.0f\n",
                camera.position.x, camera.position.y, camera.position.z,
                camera.degreesRotation.x, camera.degreesRotation.y, camera.degreesRotation.z);
        }

        CamInfoBuffer camInfo;
        camInfo.camPos = glm::vec4(camera.position, 0);
        camInfo.camRotMat = camera.camToWorldRotMat();

        drawFrame(renderer, &camInfo);
        previousFrameTime = currentTime;
    }
}

int main()
{
    char voxModelFileName[] = "scene.ply";
    MemPool<Palette> palettes(1);
    MemPool<VoxBlock> voxBlocks(144);
    VoxObject object{};
    loadPlyVoxObject(
        voxModelFileName,
        voxBlocks,
        palettes,
        &object
    );

    glfwInit();
    GLFWwindow *window = createWindow("Ray Caster", WIDTH, HEIGHT);

    Renderer renderer = createRenderer(window, enableValidationLayers);

    for(int i = 0; i < object.blockWidth * object.blockHeight * object.blockDepth; i++){
        int32_t blockIndex = object.blockIndices[i];
        if(blockIndex != 0){
            printf("%d\n", blockIndex - 1);
            updateBlock(&renderer, blockIndex - 1, voxBlocks.getBlock(blockIndex - 1));
        }
    }

    updateObject(&renderer, object);
    updatePalette(&renderer, palettes.getBlock(object.paletteIndex));

    enableStickyKeys(window);
    mainLoop(window, &renderer);

    vkDeviceWaitIdle(renderer.device);

    cleanupRenderer(&renderer);
    glfwDestroyWindow(window);
    glfwTerminate();

    palettes.cleanup();
    voxBlocks.cleanup();
    
    return EXIT_SUCCESS;
}