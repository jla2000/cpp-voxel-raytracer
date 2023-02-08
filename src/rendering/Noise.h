#pragma once

#include <string>

#include <gl/glew.h>

struct Noise {
    static Noise LoadBlueNoise(std::string const& imageDir);
    static Noise LoadWhiteNoise(int numLayers, int extent);

    GLuint textureId;
    int textureWidth;
    int textureHeight;
    int textureLayerCount;
};