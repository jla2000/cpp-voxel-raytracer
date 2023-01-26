// random stuff taken from https://www.shadertoy.com/view/tsBBWW
const float pi = 3.14159265359f;
const float twoPi = 2 * pi;

uint wangHash(inout uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

float randomFloat(inout uint state) {
    return float(wangHash(state)) / 4294967296.0;
}

vec3 randomUnitVector(inout uint state) {
    float z = randomFloat(state) * 2.0f - 1.0f;
    float a = randomFloat(state) * twoPi;
    float r = sqrt(1.0f - z * z);
    float x = r * cos(a);
    float y = r * sin(a);
    return vec3(x, y, z);
}

uint randomSeed(vec2 outputCoords, uint frame) {
    return uint(outputCoords.x) * 1973 + uint(outputCoords.y) * 9277 + frameCount * 26699 | 1;
}