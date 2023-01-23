#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

struct Camera {
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
};

glm::mat4 calculateProjection(int screenWidth, int screenHeight);
glm::mat4 calculateView(Camera& camera);