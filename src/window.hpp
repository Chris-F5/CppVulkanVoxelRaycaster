#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

GLFWwindow *createWindow(std::string title, uint32_t width, uint32_t height);
