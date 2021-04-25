#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>

#include "vulkan_device.hpp"
#include "render_pipeline.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

bool rightArrowDown = false;
bool leftArrowDown = false;
bool upArrowDown = false;
bool downArrowDown = false;
bool dKeyDown = false;
bool aKeyDown = false;
bool wKeyDown = false;
bool sKeyDown = false;

void keyCallback(GLFWwindow *_window, int key, int _scanCode, int action, int _mods)
{
    bool isPress = action == GLFW_PRESS;
    bool isRelease = action == GLFW_RELEASE;
    if (isPress || isRelease)
    {
        switch (key)
        {
        case GLFW_KEY_D:
            dKeyDown = isPress;
            break;
        case GLFW_KEY_A:
            aKeyDown = isPress;
            break;
        case GLFW_KEY_W:
            wKeyDown = isPress;
            break;
        case GLFW_KEY_S:
            sKeyDown = isPress;
            break;
        case GLFW_KEY_RIGHT:
            rightArrowDown = isPress;
            break;
        case GLFW_KEY_LEFT:
            leftArrowDown = isPress;
            break;
        case GLFW_KEY_UP:
            upArrowDown = isPress;
            break;
        case GLFW_KEY_DOWN:
            downArrowDown = isPress;
            break;
        }
    }
}

class App
{
public:
    void run()
    {
        initWindow();
        renderPipeline = createRenderPipeline(window, enableValidationLayers, "shader.spv");
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow *window;
    RenderPipeline renderPipeline;
    glm::vec4 cameraPosition = glm::vec4(0.0);
    glm::vec2 cameraRot = glm::vec2(0.0);

    void initWindow()
    {
        glfwInit();

        // Dont use opengl
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulcan Test App", nullptr, nullptr);

        glfwSetKeyCallback(window, keyCallback);
    }
    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            if (dKeyDown)
            {
                cameraPosition.x += 0.01;
            }
            if (aKeyDown)
            {
                cameraPosition.x -= 0.01;
            }
            if (wKeyDown)
            {
                cameraPosition.y += 0.01;
            }
            if (sKeyDown)
            {
                cameraPosition.y -= 0.01;
            }

            if (rightArrowDown)
            {
                cameraRot.y += 0.1;
            }
            if (leftArrowDown)
            {
                cameraRot.y -= 0.1;
            }
            if (upArrowDown)
            {
                cameraRot.x -= 0.1;
            }
            if (downArrowDown)
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

            drawFrame(&renderPipeline, camInfo);

            //std::cout << cameraPosition.x << " " << cameraPosition.y << std::endl;
        }

        vkDeviceWaitIdle(renderPipeline.device);
    }

    void cleanup()
    {
        cleanupRenderPipeline(&renderPipeline);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main()
{
    App app;

    try
    {
        app.run();
    }
    catch (const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}