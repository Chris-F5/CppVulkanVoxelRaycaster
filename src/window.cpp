#include "window.hpp"

GLFWwindow *createWindow(std::string title, uint32_t width, uint32_t height)
{
    // Dont use opengl
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    return glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}