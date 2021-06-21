#include "camera_controller.hpp"
#include <iostream>

glm::vec3 Camera::radianRotation()
{
    glm::vec3 radianRot;
    radianRot.x = glm::radians(degreesRotation.x);
    radianRot.y = glm::radians(degreesRotation.y);
    radianRot.z = glm::radians(degreesRotation.z);
    return radianRot;
}

glm::vec3 Camera::forwardDirection()
{
    glm::vec3 radianRot = radianRotation();

    glm::vec3 direction;
    direction.z = cos(radianRot.x) * cos(radianRot.y);
    direction.y = sin(radianRot.y);
    direction.x = sin(radianRot.x) * cos(radianRot.y);
    return glm::normalize(direction);
}

glm::vec3 Camera::rightDirection()
{
    return glm::cross(glm::vec3(0, 1, 0), forwardDirection());
}

glm::mat4 Camera::camToWorldRotMat()
{
    glm::vec3 radianRot = radianRotation();

    glm::quat camRotation;
    camRotation = glm::quat(glm::vec3(-radianRot.y, radianRot.x, radianRot.z));

    return glm::toMat4(camRotation);
}

void updateCamera(Camera *camera, InputState inputState, float deltaTime)
{
    if (inputState.d)
    {
        camera->position += camera->rightDirection() * camera->speed * deltaTime;
    }
    if (inputState.a)
    {
        camera->position -= camera->rightDirection() * camera->speed * deltaTime;
    }
    if (inputState.w)
    {
        camera->position += camera->forwardDirection() * camera->speed * deltaTime;
    }
    if (inputState.s)
    {
        camera->position -= camera->forwardDirection() * camera->speed * deltaTime;
    }
    if (inputState.space)
    {
        camera->position.y += camera->speed * deltaTime;
    }
    if (inputState.leftShift)
    {
        camera->position.y -= camera->speed * deltaTime;
    }

    if (inputState.rightArrow)
    {
        camera->degreesRotation.x += camera->rotateSpeed * deltaTime;
    }
    if (inputState.leftArrow)
    {
        camera->degreesRotation.x -= camera->rotateSpeed* deltaTime;
    }
    if (inputState.upArrow)
    {
        camera->degreesRotation.y += camera->rotateSpeed * deltaTime;
    }
    if (inputState.downArrow)
    {
        camera->degreesRotation.y -= camera->rotateSpeed * deltaTime;
    }
}