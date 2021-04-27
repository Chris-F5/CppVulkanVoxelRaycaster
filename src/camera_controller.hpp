#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "input.hpp"

struct Camera
{
    glm::vec3 position;
    glm::vec3 degreesRotation;

    glm::vec3 radianRotation();
    glm::mat4 camToWorldRotMat();
};

void updateCamera(Camera *camera, InputState inputState);