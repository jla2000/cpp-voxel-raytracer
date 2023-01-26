// Sky color example from Ray Tracing In One Weekend
vec3 skyColor(vec3 rayDir) {
    float t = 0.5 * (rayDir.y + 1);
    return (1 - t) * vec3(1) + t * vec3(0.5, 0.7, 1.0);
}
