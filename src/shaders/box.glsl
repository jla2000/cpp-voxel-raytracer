vec2 intersectBox(vec3 rayPos, vec3 invRayDir, vec3 boxMin, vec3 boxMax) {
    vec3 tMin = (boxMin - rayPos) * invRayDir;
    vec3 tMax = (boxMax - rayPos) * invRayDir;

    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);

    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar  = min(min(t2.x, t2.y), t2.z);

    return vec2(tNear, tFar);
}
