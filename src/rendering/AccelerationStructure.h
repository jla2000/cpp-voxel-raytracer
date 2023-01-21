#pragma once

#include <vector>

#include <glm/vec3.hpp>
#include <gl/glew.h>

struct AccelerationStructure {
    AccelerationStructure(const glm::vec3& center, GLfloat extent);
    AccelerationStructure(const glm::vec3& vMin, const glm::vec3& vMax);

    std::vector<GLfloat> vertices;
    std::vector<GLuint> triangles;
};