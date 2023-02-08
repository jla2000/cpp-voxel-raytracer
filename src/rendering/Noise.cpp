#include "Noise.h"

#include <vector>
#include <filesystem>
#include <random>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Noise Noise::LoadBlueNoise(std::string const& imageDir) {
    Noise noise{};

    std::vector<unsigned char*> imageBuffers;
    int width, height, channels;
    for (auto const& entry : std::filesystem::directory_iterator(imageDir)) {
        imageBuffers.push_back(stbi_load(entry.path().string().c_str(), &width, &height, &channels, 0));
    }

    noise.textureWidth = width;
    noise.textureHeight = height;
    noise.textureLayerCount = imageBuffers.size();

    glGenTextures(1, &noise.textureId);
    glBindTexture(GL_TEXTURE_2D_ARRAY, noise.textureId);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, noise.textureWidth, noise.textureHeight, noise.textureLayerCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    int zOffset = 0;
    for (unsigned char* imageBuffer : imageBuffers) {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, zOffset++, noise.textureWidth, noise.textureHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
        free(imageBuffer);
    }
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    return noise;
}

Noise Noise::LoadWhiteNoise(int numLayers, int extent) {
    Noise noise{};
    noise.textureWidth = extent;
    noise.textureHeight = extent;
    noise.textureLayerCount = numLayers;

    std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char> engine{};
    std::vector<unsigned char> randomBytes(extent * extent * numLayers * 2);
    std::generate(randomBytes.begin(), randomBytes.end(), engine);

    glGenTextures(1, &noise.textureId);
    glBindTexture(GL_TEXTURE_2D_ARRAY, noise.textureId);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, noise.textureWidth, noise.textureHeight, noise.textureLayerCount, 0, GL_RG, GL_UNSIGNED_BYTE, randomBytes.data());
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    return noise;
}
