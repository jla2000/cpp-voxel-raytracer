#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

struct Camera {
    Camera(glm::vec3 position, glm::vec3 focusPoint, int screenWidth, int screenHeight);

    void updateView();
    void updateProjection(int screenWidth, int screenHeight);
    void arcBallRotate(float deltaXPixels, float deltaYPixels, float screenWidth, float screenHeight);

    glm::vec3 m_position;
    glm::vec3 m_focusPoint;
    glm::vec3 m_upVector;
    glm::mat4 m_viewMat;
    glm::mat4 m_invViewMat;
    glm::mat4 m_invCenteredMat;
    glm::mat4 m_projectionMat;
    glm::mat4 m_invProjectionMat;
};