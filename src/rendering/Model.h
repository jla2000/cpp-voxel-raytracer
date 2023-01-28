#pragma once

#include <array>
#include <vector>
#include <string>

#include <glm/vec3.hpp>

struct Model {
    glm::uvec3 size;
    std::array<uint32_t, 256> palette;
    std::vector<uint8_t> indices;
    std::vector<uint8_t> solid;
};

Model loadExampleModel();
Model loadVoxModel(const std::string& filename);