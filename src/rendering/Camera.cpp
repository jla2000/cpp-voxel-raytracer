#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Camera::Camera(glm::vec3 position, glm::vec3 focusPoint, int screenWidth, int screenHeight) {
    m_position = position;
    m_focusPoint = focusPoint;
    m_upVector = {0, 1, 0};

    updateView();
    updateProjection(screenWidth, screenHeight);
}

void Camera::updateView() {
    m_viewMat = glm::lookAt(
            m_position,
            m_focusPoint,
            m_upVector
    );
    m_invViewMat = glm::inverse(m_viewMat);
    m_invCenteredMat = m_invViewMat;
    m_invCenteredMat[3][0] = 0;
    m_invCenteredMat[3][1] = 0;
    m_invCenteredMat[3][2] = 0;
}

void Camera::updateProjection(int screenWidth, int screenHeight) {
    //float w = float(screenWidth) / 50;
    //float h = float(screenHeight) / 50;
    //m_projectionMat = glm::ortho(-w, w, -h, h, 0.1f, 1.0f);

    m_projectionMat = glm::perspective(
            glm::radians(45.0f),
            float(screenWidth) / float(screenHeight),
            0.1f,
            100.0f
    );
    m_invProjectionMat = glm::inverse(m_projectionMat);
}

void Camera::arcBallRotate(float deltaXPixels, float deltaYPixels, float screenWidth, float screenHeight) {
    glm::vec4 position = glm::vec4(m_position, 1.0f);
    glm::vec4 pivot = glm::vec4(m_focusPoint, 1.0f);
    glm::vec3 right = glm::transpose(m_viewMat)[0];

    float deltaAngleX = (2 * M_PI / screenWidth);
    float deltaAngleY = (M_PI / screenHeight);
    float xAngle = deltaXPixels * deltaAngleX;
    float yAngle = deltaYPixels * deltaAngleY;

    glm::vec3 viewDir = -glm::transpose(m_viewMat)[2];
    float cosAngle = dot(viewDir, m_upVector);
    if (cosAngle * glm::sign(deltaAngleY) > 0.99f)
        deltaAngleY = 0;

    glm::mat4 rotationX(1.0f);
    rotationX = glm::rotate(rotationX, xAngle, m_upVector);
    position = (rotationX * (position - pivot)) + pivot;

    glm::mat4 rotationY(1.0f);
    rotationY = glm::rotate(rotationY, yAngle, right);
    position = (rotationY * (position - pivot)) + pivot;

    m_position = position;

    updateView();
}