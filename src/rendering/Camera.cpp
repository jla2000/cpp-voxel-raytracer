#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 calculateProjection(int screenWidth, int screenHeight) {
    return glm::perspective(
        glm::radians(45.0f),
        float(screenWidth) / float(screenHeight),
        0.1f,
        100.0f
    );
    //float halfW = float(screenWidth) / 2.0f;
    //float halfH = float(screenHeight) / 2.0f;
    //return glm::ortho(-halfW, halfW, -halfH, halfH);
    //float w = float(screenWidth);
    //float h = float(screenHeight);
    //return glm::ortho(0.0f, w, 0.0f, h, 0.1f, 100.0f);
}

glm::mat4 calculateView(Camera& camera) {
    return glm::lookAt(
        camera.position,
        camera.target,
        glm::vec3(0, 1, 0)
    );

    /*return glm::lookAt(
            {63, 63, 63},
            {0, 0, 0},
            glm::vec3(0, 1, 0)
            );*/
}