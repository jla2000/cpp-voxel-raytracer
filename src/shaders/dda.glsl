struct DDA {
    vec3 pos;
    vec3 rayStep;
    vec3 deltaDist;
    vec3 sideDist;
    vec3 normal;
    float dist;
};

void initDDA(out DDA dda, vec3 rayPos, vec3 rayDir) {
    dda.pos = floor(rayPos);
    dda.rayStep = sign(rayDir);
    dda.deltaDist = dda.rayStep / rayDir;
    dda.sideDist = (sign(rayDir) * (dda.pos - rayPos) + (sign(rayDir) * 0.5) + 0.5) * dda.deltaDist;
}

void iterDDA(inout DDA dda) {
    //bvec3 mask = lessThanEqual(dda.sideDist.xyz, min(dda.sideDist.yzx, dda.sideDist.zxy));
    //dda.sideDist += vec3(mask) * dda.deltaDist;
    //dda.pos += vec3(mask) * dda.rayStep;
    //dda.normal = vec3(mask) * -dda.rayStep;
    // FIXME: branchless dda normals

    if (dda.sideDist.x < dda.sideDist.y) {
        if (dda.sideDist.z < dda.sideDist.x) {
            dda.dist = dda.sideDist.z;
            dda.sideDist.z += dda.deltaDist.z;
            dda.pos.z += dda.rayStep.z;
            dda.normal = vec3(0, 0,-dda.rayStep.z);
        } else {
            dda.dist = dda.sideDist.x;
            dda.sideDist.x += dda.deltaDist.x;
            dda.pos.x += dda.rayStep.x;
            dda.normal = vec3(-dda.rayStep.x, 0, 0);
        }
    } else {
        if (dda.sideDist.z < dda.sideDist.y) {
            dda.dist = dda.sideDist.z;
            dda.sideDist.z += dda.deltaDist.z;
            dda.pos.z += dda.rayStep.z;
            dda.normal = vec3(0, 0,-dda.rayStep.z);
        } else {
            dda.dist = dda.sideDist.y;
            dda.sideDist.y += dda.deltaDist.y;
            dda.pos.y += dda.rayStep.y;
            dda.normal = vec3(0, -dda.rayStep.y, 0);
        }
    }
}