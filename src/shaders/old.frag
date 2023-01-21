#version 440 core

in mat4 inverseMVP;
out vec4 color;

uniform vec3 cameraPositionWorldSpace;
uniform float screenWidth;
uniform float screenHeight;

struct Ray {
    vec3 origin;
    vec3 direction;
};

vec3 walkRay(Ray ray, float distance) {
    return ray.origin + ray.direction * distance;
}

float intersectSphere(vec3 center, float radius, Ray ray) {
    vec3 oc = ray.origin - center;
    float a = dot(ray.direction, ray.direction);
    float b = 2 * dot(oc, ray.direction);
    float c = dot(oc, oc) - radius*radius;
    float discriminant = b*b - 4*a*c;

    if (discriminant < 0) {
        return -1;
    } else {
        return (-b - sqrt(discriminant)) / (2*a);
    }
}

vec2 intersectBox(vec3 boxMin, vec3 boxMax, Ray ray) {
    vec3 tMin = (boxMin - ray.origin) / ray.direction;
    vec3 tMax = (boxMax - ray.origin) / ray.direction;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);

    return vec2(tNear, tFar);
}

vec2 intersectVoxel(vec3 center, Ray ray) {
    float voxelSize = 0.05;
    vec3 dims = vec3(voxelSize, voxelSize, voxelSize);
    vec3 boxMin = center - dims;
    vec3 boxMax = center + dims;

    return intersectBox(boxMin, boxMax, ray);
}

vec3 rayColor(Ray ray) {
    vec3 color = vec3(0.5, 0.2, 0.4);
    float distance = 1.0/0.0;

    vec3 t1Pos = vec3(1, 0, 0);
    vec3 t2Pos = vec3(0, 0, 0);
    float t1 = intersectSphere(t1Pos, 0.5, ray);
    float t2 = intersectSphere(t2Pos, 0.25, ray);

    if (t1 > 0 && t1 < distance) {
        distance = t1;
        color = vec3(0.1, 0.1, 0.1);
    }

    if (t2 > 0 && t2 < distance) {
        distance = t2;
        color = vec3(0.9, 0.9, 0.9);
    }

    //if (isinf(distance)) {
    //    discard;
    //}

    return color;
}

void main() {
    vec4 ndc = vec4(
    (gl_FragCoord.x / screenWidth - 0.5) * 2,
    (gl_FragCoord.y / screenHeight - 0.5) * 2,
    (gl_FragCoord.z - 0.5) * 2,
    1
    );

    vec4 clip = inverseMVP * ndc;
    vec4 hit = vec4((clip / clip.w).xyz, 1);

    Ray eye;
    eye.origin = cameraPositionWorldSpace;
    eye.direction = normalize(hit.xyz - eye.origin);

    color = vec4(rayColor(eye), 1.0);
}