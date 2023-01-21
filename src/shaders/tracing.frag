#version 440 core

in mat4 inverseMvp;
out vec4 color;

uniform vec3 cameraPositionWorldSpace;
uniform float screenWidth;
uniform float screenHeight;
uniform vec3 voxelBuffer[8*8*8];

const float voxelSize = 1;
const float halfVoxelExtent = voxelSize / 2.0;

struct Ray {
    vec3 origin;
    vec3 direction;
};

bool inVoxelBuffer(ivec3 vx) {
    return (vx.x >= 0 && vx.x < 8) &&
           (vx.y >= 0 && vx.y < 8) &&
           (vx.z >= 0 && vx.z < 8);
}

vec3 getVoxel(ivec3 vx) {
    if (!inVoxelBuffer(vx)) {
        return vec3(0, 0, 0);
    }
    return voxelBuffer[vx.z * 8 * 8 + vx.y * 8 + vx.x];
}

vec3 intersectVoxel(Ray ray) {
    vec3 pos = floor(ray.origin);
    vec3 step = sign(ray.direction);
    vec3 tDelta = step / ray.direction;

    float tMaxX, tMaxY, tMaxZ;
    vec3 fr = fract(ray.origin);

    tMaxX = tDelta.x * ((ray.direction.x > 0) ? (1.0 - fr.x) : fr.x);
    tMaxY = tDelta.y * ((ray.direction.y > 0) ? (1.0 - fr.y) : fr.y);
    tMaxZ = tDelta.z * ((ray.direction.z > 0) ? (1.0 - fr.z) : fr.z);

    vec3 norm;
    const int maxTrace = 100;

    for (int i = 0; i < maxTrace; ++i) {
        vec3 voxel = getVoxel(ivec3(pos));
        if (voxel != vec3(0)) {
            return voxel;
        }

        if (tMaxX < tMaxY) {
            if (tMaxZ < tMaxX) {
                tMaxZ += tDelta.z;
                pos.z += step.z;
                norm = vec3(0, 0,-step.z);
            } else {
                tMaxX += tDelta.x;
                pos.x += step.x;
                norm = vec3(-step.x, 0, 0);
            }
        } else {
            if (tMaxZ < tMaxY) {
                tMaxZ += tDelta.z;
                pos.z += step.z;
                norm = vec3(0, 0, -step.z);
            } else {
                tMaxY += tDelta.y;
                pos.y += step.y;
                norm = vec3(0, -step.y, 0);
            }
        }
    }

    return vec3(1);
}

void main() {
    vec4 ndc = vec4(
        (gl_FragCoord.x / screenWidth - 0.5) * 2,
        (gl_FragCoord.y / screenHeight - 0.5) * 2,
        (gl_FragCoord.z - 0.5) * 2,
        1
    );

    vec4 clip = inverseMvp * ndc;
    vec4 hit = vec4((clip / clip.w).xyz, 1);

    Ray eye;
    eye.origin = cameraPositionWorldSpace;
    eye.direction = normalize(hit.xyz - eye.origin);

    color = vec4(intersectVoxel(eye), 1);
}