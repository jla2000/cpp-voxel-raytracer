#pragma once

#include <gl/glew.h>
#include <glm/vec3.hpp>

struct VoxelBuffer {
    VoxelBuffer();
    glm::vec3& voxel(int x, int y, int z);

    glm::vec3 data[8*8*8];
};