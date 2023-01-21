#include "AccelerationStructure.h"

struct CubeFace {
    glm::vec3 vertices[4];
};

struct CubeData {
    std::vector<CubeFace> faces;
};

AccelerationStructure::AccelerationStructure(const glm::vec3& center, GLfloat extent)
        : AccelerationStructure(
        center - glm::vec3(extent / 2.0f),
        center + glm::vec3(extent / 2.0f)
        )
{}

AccelerationStructure::AccelerationStructure(const glm::vec3 &vMin, const glm::vec3 &vMax) {
    const CubeFace frontFace{{
        {vMin.x, vMax.y, vMax.z},
        {vMax.x, vMax.y, vMax.z},
        {vMax.x, vMin.y, vMax.z},
        {vMin.x, vMin.y, vMax.z}
    }};

    const CubeFace rightFace{{
        {vMax.x, vMax.y, vMax.z},
        {vMax.x, vMax.y, vMin.z},
        {vMax.x, vMin.y, vMin.z},
        {vMax.x, vMin.y, vMax.z}
    }};

    const CubeFace backFace{{
        {vMax.x, vMax.y, vMin.z},
        {vMin.x, vMax.y, vMin.z},
        {vMin.x, vMin.y, vMin.z},
        {vMax.x, vMin.y, vMin.z}
    }};

    const CubeFace leftFace{{
        {vMin.x, vMax.y, vMin.z},
        {vMin.x, vMax.y, vMax.z},
        {vMin.x, vMin.y, vMax.z},
        {vMin.x, vMin.y, vMin.z}
    }};

    const CubeFace topFace{{
        {vMin.x, vMax.y, vMin.z},
        {vMax.x, vMax.y, vMin.z},
        {vMax.x, vMax.y, vMax.z},
        {vMin.x, vMax.y, vMax.z}
    }};

    const CubeFace bottomFace{{
        {vMin.x, vMin.y, vMax.z},
        {vMax.x, vMin.y, vMax.z},
        {vMax.x, vMin.y, vMin.z},
        {vMin.x, vMin.y, vMin.z}
    }};

    const CubeData cubeData{{
        frontFace,
        rightFace,
        backFace,
        leftFace,
        topFace,
        bottomFace
    }};

    for (const auto &face: cubeData.faces) {
        for (const auto &vertex: face.vertices) {
            vertices.insert(vertices.end(), {
                    vertex.x,
                    vertex.y,
                    vertex.z,
            });
        }

        const GLuint numVertices = vertices.size();
        triangles.insert(triangles.end(), {
                numVertices / 3 - 4,
                numVertices / 3 - 3,
                numVertices / 3 - 2,
                numVertices / 3 - 4,
                numVertices / 3 - 2,
                numVertices / 3 - 1,
        });
    }
}