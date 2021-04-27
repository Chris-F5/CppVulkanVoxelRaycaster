#include "camera_controller.hpp"

glm::vec3 Camera::radianRotation()
{
    glm::vec3 radianRot;
    radianRot.x = glm::radians(degreesRotation.x);
    radianRot.y = glm::radians(degreesRotation.y);
    radianRot.z = glm::radians(degreesRotation.z);
    return radianRot;
}

glm::mat4 Camera::camToWorldRotMat()
{
    glm::vec3 radianRot = radianRotation();

    glm::quat camRotation;
    camRotation = glm::quat(radianRot);

    return glm::toMat4(camRotation);
}

void updateCamera(Camera *camera, InputState inputState)
{
    if (inputState.d)
    {
        camera->position.x += 0.01;
    }
    if (inputState.a)
    {
        camera->position.x -= 0.01;
    }
    if (inputState.w)
    {
        camera->position.y += 0.01;
    }
    if (inputState.s)
    {
        camera->position.y -= 0.01;
    }

    if (inputState.rightArrow)
    {
        camera->degreesRotation.y += 0.1;
    }
    if (inputState.leftArrow)
    {
        camera->degreesRotation.y -= 0.1;
    }
    if (inputState.upArrow)
    {
        camera->degreesRotation.x -= 0.1;
    }
    if (inputState.downArrow)
    {
        camera->degreesRotation.x += 0.1;
    }
}