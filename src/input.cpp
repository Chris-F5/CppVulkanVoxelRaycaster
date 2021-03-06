#include "input.hpp"
#include <stdexcept>
#include <iostream>

bool isKeyDown(GLFWwindow *window, int key)
{
    return glfwGetKey(window, key) == GLFW_PRESS;
}

InputState pollInput(GLFWwindow *window)
{
    InputState state;
    state.rightArrow = isKeyDown(window, GLFW_KEY_RIGHT);
    state.leftArrow = isKeyDown(window, GLFW_KEY_LEFT);
    state.upArrow = isKeyDown(window, GLFW_KEY_UP);
    state.downArrow = isKeyDown(window, GLFW_KEY_DOWN);

    state.d = isKeyDown(window, GLFW_KEY_D);
    state.a = isKeyDown(window, GLFW_KEY_A);
    state.w = isKeyDown(window, GLFW_KEY_W);
    state.s = isKeyDown(window, GLFW_KEY_S);

    state.leftShift = isKeyDown(window, GLFW_KEY_LEFT_SHIFT);
    state.space = isKeyDown(window, GLFW_KEY_SPACE);

    state.p = isKeyDown(window, GLFW_KEY_P);

    return state;
}

void enableStickyKeys(GLFWwindow *window)
{
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
}