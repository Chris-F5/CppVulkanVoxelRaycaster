#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>

#include "vulkan_device.hpp"
#include "render_pipeline.hpp"
#include "input.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

GLFWwindow *createWindow(std::string title, uint32_t width, uint32_t height)
{
    // Dont use opengl
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    return glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}

void mainLoop(GLFWwindow *window, RenderPipeline *renderPipeline)
{
    glm::vec4 cameraPosition = glm::vec4(0.0);
    glm::vec2 cameraRot = glm::vec2(0.0);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        InputState inputState = pollInput(window);

        if (inputState.d)
        {
            cameraPosition.x += 0.01;
        }
        if (inputState.a)
        {
            cameraPosition.x -= 0.01;
        }
        if (inputState.w)
        {
            cameraPosition.y += 0.01;
        }
        if (inputState.s)
        {
            cameraPosition.y -= 0.01;
        }

        if (inputState.rightArrow)
        {
            cameraRot.y += 0.1;
        }
        if (inputState.leftArrow)
        {
            cameraRot.y -= 0.1;
        }
        if (inputState.upArrow)
        {
            cameraRot.x -= 0.1;
        }
        if (inputState.downArrow)
        {
            cameraRot.x += 0.1;
        }

        UniformData camInfo;
        camInfo.camPos = cameraPosition;
        // ======== MAKE ROT MATRIX
        float pi = 3.141592;
        // camRotX and Y are defined elsewhere and can be controlled from the keyboard during runtime.
        glm::vec3 camEulerAngles = glm::vec3(cameraRot.x, cameraRot.y, 0);

        // Convert to radians
        camEulerAngles.x = camEulerAngles.x * pi / 180;
        camEulerAngles.y = camEulerAngles.y * pi / 180;
        camEulerAngles.z = camEulerAngles.z * pi / 180;

        // Generate Quaternian
        glm::quat camRotation;
        camRotation = glm::quat(camEulerAngles);

        // Generate rotation matrix from quaternian
        glm::mat4 camToWorldMatrix = glm::toMat4(camRotation);
        // =======
        camInfo.camRotMat = camToWorldMatrix;

        drawFrame(renderPipeline, camInfo);
    }
}

int main()
{
    glfwInit();
    GLFWwindow *window = createWindow("Vulkan Test App", WIDTH, HEIGHT);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

    RenderPipeline renderPipeline = createRenderPipeline(window, enableValidationLayers, "shader.spv");

    mainLoop(window, &renderPipeline);

    vkDeviceWaitIdle(renderPipeline.device);

    cleanupRenderPipeline(&renderPipeline);
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}