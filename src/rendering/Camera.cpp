#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 calculateProjection(int screenWidth, int screenHeight) {
    //float w = float(screenWidth) / 50;
    //float h = float(screenHeight) / 50;
    //return glm::ortho(-w, w, -h, h, 0.1f, 1.0f);

    return glm::perspective(
        glm::radians(45.0f),
        float(screenWidth) / float(screenHeight),
        0.1f,
        100.0f
    );
}

glm::mat4 calculateView(Camera& camera) {
    return glm::lookAt(
        camera.position,
        camera.target,
        camera.up
    );
}