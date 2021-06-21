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
    bool leftShift = false;
    bool space = false;
    bool p = false;
};

InputState pollInput(GLFWwindow *window);
void enableStickyKeys(GLFWwindow *window);