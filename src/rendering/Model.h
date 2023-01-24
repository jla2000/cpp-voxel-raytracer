#pragma once

#include <array>
#include <vector>
#include <string>

#include <glm/vec3.hpp>

struct Model {
    glm::uvec3 size;
    std::array<uint32_t, 256> palette;
    std::vector<uint8_t> indices;
};

Model demoModel();
Model loadModel(const std::string& filename);