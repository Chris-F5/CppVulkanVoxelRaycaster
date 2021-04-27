#pragma once

#include <GLFW/glfw3.h>

struct InputState
{
    bool rightArrow = false;
    bool leftArrow = false;
    bool upArrow = false;
    bool downArrow = false;
    bool d = false;
    bool a = false;
    bool w = false;
    bool s = false;
};

InputState pollInput(GLFWwindow *window);