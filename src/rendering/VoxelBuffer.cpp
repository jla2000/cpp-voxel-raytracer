#include "VoxelBuffer.h"

#include <cstring>

VoxelBuffer::VoxelBuffer() {
    std::memset(&data[0][0], 0, 3*8*8*8);
}

glm::vec3 &VoxelBuffer::voxel(int x, int y, int z) {
    return data[z * 8 * 8 + y * 8 + x];
}