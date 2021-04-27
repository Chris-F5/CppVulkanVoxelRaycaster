#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "input.hpp"

struct Camera
{
    float speed;
    glm::vec3 position;
    glm::vec3 degreesRotation;

    glm::vec3 radianRotation();
    glm::vec3 forwardDirection();
    glm::vec3 rightDirection();
    glm::mat4 camToWorldRotMat();
};

void updateCamera(Camera *camera, InputState inputState);