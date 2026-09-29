const char* kEmbeddedFrag = R"GLSL(
// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
#version 330 core
// Full raytracer in a fragment shader. No scene meshes — pure analytic intersections.
in vec2 v_uv;
out vec4 fragColor;

#define MAX_SPHERES 16
#define MAX_BOXES   128
#define MAX_PLANES  12

uniform vec3 u_camPos;
uniform vec3 u_camForward;
uniform vec3 u_camRight;
uniform vec3 u_camUp;
uniform float u_fovScale;
uniform float u_aspect;

uniform vec3 u_lightPos;
uniform vec3 u_lightColor;
uniform vec3 u_ambient;
uniform vec3 u_skyColor;
uniform int u_maxBounces;
uniform int u_shadowSamples;
uniform float u_exposure;

uniform int u_numSpheres;
uniform vec3 u_sphereCenter[MAX_SPHERES];
uniform float u_sphereRadius[MAX_SPHERES];
uniform vec3 u_sphereColor[MAX_SPHERES];
uniform float u_sphereReflect[MAX_SPHERES];

uniform int u_numBoxes;
uniform vec3 u_boxMin[MAX_BOXES];
uniform vec3 u_boxMax[MAX_BOXES];
uniform vec3 u_boxColor[MAX_BOXES];
uniform float u_boxReflect[MAX_BOXES];

uniform int u_numPlanes;
uniform vec3 u_planePoint[MAX_PLANES];
uniform vec3 u_planeNormal[MAX_PLANES];
uniform vec3 u_planeColorA[MAX_PLANES];
uniform vec3 u_planeColorB[MAX_PLANES];
uniform float u_planeScale[MAX_PLANES];
uniform float u_planeReflect[MAX_PLANES];
uniform int u_planeChecker[MAX_PLANES];

struct Hit {
    float t;
    vec3 point;
    vec3 normal;
    vec3 color;
    float reflectivity;
    bool hit;
};

Hit intersect(vec3 ro, vec3 rd) {
    Hit best;
    best.t = 1e30;
    best.hit = false;
    best.color = vec3(0.0);
    best.reflectivity = 0.0;
    best.normal = vec3(0.0);
    best.point = vec3(0.0);

    for (int i = 0; i < MAX_SPHERES; ++i) {
        if (i >= u_numSpheres) break;
        vec3 oc = ro - u_sphereCenter[i];
        float b = dot(oc, rd);
        float c = dot(oc, oc) - u_sphereRadius[i] * u_sphereRadius[i];
        float disc = b * b - c;
        if (disc < 0.0) continue;
        float sq = sqrt(disc);
        float t = -b - sq;
        if (t < 1e-4) t = -b + sq;
        if (t > 1e-4 && t < best.t) {
            best.t = t;
            best.point = ro + rd * t;
            best.normal = normalize(best.point - u_sphereCenter[i]);
            best.color = u_sphereColor[i];
            best.reflectivity = u_sphereReflect[i];
            best.hit = true;
        }
    }

    for (int i = 0; i < MAX_BOXES; ++i) {
        if (i >= u_numBoxes) break;
        vec3 invDir = 1.0 / rd;
        vec3 t0 = (u_boxMin[i] - ro) * invDir;
        vec3 t1 = (u_boxMax[i] - ro) * invDir;
        vec3 tmin3 = min(t0, t1);
        vec3 tmax3 = max(t0, t1);
        float tmin = max(max(tmin3.x, tmin3.y), tmin3.z);
        float tmax = min(min(tmax3.x, tmax3.y), tmax3.z);
        if (tmax < 0.0 || tmin > tmax) continue;
        float t = tmin > 1e-4 ? tmin : tmax;
        if (t < 1e-4 || t >= best.t) continue;

        best.t = t;
        best.point = ro + rd * t;
        vec3 center = (u_boxMin[i] + u_boxMax[i]) * 0.5;
        vec3 d = best.point - center;
        vec3 halfExtent = (u_boxMax[i] - u_boxMin[i]) * 0.5;
        best.normal = normalize(vec3(
            float(int(d.x / abs(halfExtent.x) * 1.0001)),
            float(int(d.y / abs(halfExtent.y) * 1.0001)),
            float(int(d.z / abs(halfExtent.z) * 1.0001))));
        best.color = u_boxColor[i];
        best.reflectivity = u_boxReflect[i];
        best.hit = true;
    }

    for (int i = 0; i < MAX_PLANES; ++i) {
        if (i >= u_numPlanes) break;
        float denom = dot(u_planeNormal[i], rd);
        if (abs(denom) < 1e-6) continue;
        float t = dot(u_planePoint[i] - ro, u_planeNormal[i]) / denom;
        if (t < 1e-4 || t >= best.t) continue;

        best.t = t;
        best.point = ro + rd * t;
        best.normal = denom < 0.0 ? u_planeNormal[i] : -u_planeNormal[i];
        best.reflectivity = u_planeReflect[i];

        if (u_planeChecker[i] != 0) {
            float u, v;
            if (abs(u_planeNormal[i].y) > 0.9) {
                u = best.point.x;
                v = best.point.z;
            } else if (abs(u_planeNormal[i].x) > 0.9) {
                u = best.point.z;
                v = best.point.y;
            } else {
                u = best.point.x;
                v = best.point.y;
            }
            int iu = int(floor(u * u_planeScale[i]));
            int iv = int(floor(v * u_planeScale[i]));
            best.color = ((iu + iv) & 1) != 0 ? u_planeColorA[i] : u_planeColorB[i];
        } else {
            best.color = u_planeColorA[i];
        }
        best.hit = true;
    }

    return best;
}

// Local lighting for a single hit (no recursion — Mesa forbids recursive GLSL).
// Soft shadow: up to 8 fixed disk samples; u_shadowSamples selects how many.
float softShadow(vec3 p, vec3 n) {
    float lit = 0.0;
    vec3 offsets[8];
    offsets[0] = vec3( 0.35, 0.0,  0.15);
    offsets[1] = vec3(-0.25, 0.1, -0.30);
    offsets[2] = vec3( 0.10, 0.0, -0.35);
    offsets[3] = vec3(-0.15, 0.05, 0.40);
    offsets[4] = vec3( 0.40, 0.05, -0.10);
    offsets[5] = vec3(-0.40, 0.0,  0.20);
    offsets[6] = vec3( 0.05, 0.1,  0.45);
    offsets[7] = vec3(-0.05, 0.0, -0.45);
    float lightRadius = 0.55;
    int samples = clamp(u_shadowSamples, 1, 8);
    for (int i = 0; i < 8; ++i) {
        if (i >= samples) break;
        vec3 lp = u_lightPos + offsets[i] * lightRadius;
        vec3 toL = lp - p;
        float dist = length(toL);
        toL /= max(dist, 1e-4);
        Hit sh = intersect(p + n * 1e-3, toL);
        if (!(sh.hit && sh.t < dist)) {
            lit += 1.0;
        }
    }
    return mix(0.22, 1.0, lit / float(samples));
}

vec3 shadeHit(Hit h, vec3 rd) {
    vec3 col = h.color * u_ambient;

    vec3 toLight = normalize(u_lightPos - h.point);
    float ndotl = max(0.0, dot(h.normal, toLight));

    float shadowFactor = softShadow(h.point, h.normal);

    col += h.color * u_lightColor * ndotl * shadowFactor;

    vec3 viewDir = -rd;
    vec3 halfV = normalize(toLight + viewDir);
    float spec = pow(max(0.0, dot(h.normal, halfV)), 32.0);
    col += u_lightColor * (spec * 0.4 * shadowFactor);

    return col;
}

// Iterative path tracer for primary + reflection bounces.
vec3 trace(vec3 ro, vec3 rd) {
    vec3 throughput = vec3(1.0);
    vec3 result = vec3(0.0);

    int maxB = clamp(u_maxBounces, 0, 3);
    for (int bounce = 0; bounce <= 3; ++bounce) {
        if (bounce > maxB) {
            break;
        }
        Hit h = intersect(ro, rd);
        if (!h.hit) {
            result += throughput * u_skyColor;
            break;
        }

        vec3 local = shadeHit(h, rd);
        float kr = h.reflectivity;
        result += throughput * local * (1.0 - kr);

        if (kr < 0.01 || bounce == maxB) {
            break;
        }

        // Continue along reflection
        throughput *= kr;
        rd = reflect(rd, h.normal);
        ro = h.point + h.normal * 1e-3;
    }

    return clamp(result, 0.0, 1.0);
}

void main() {
    // v_uv is 0..1; convert to NDC-style ray
    float u = (2.0 * v_uv.x - 1.0) * u_aspect * u_fovScale;
    float v = (2.0 * v_uv.y - 1.0) * u_fovScale; // OpenGL y already bottom-up in our UV

    vec3 rd = normalize(u_camForward + u_camRight * u + u_camUp * v);
    vec3 col = trace(u_camPos, rd);

    // Soft 90s-style vignette
    vec2 n = v_uv * 2.0 - 1.0;
    float vig = clamp(1.0 - 0.35 * dot(n, n), 0.0, 1.0);
    col *= vig;

    // Exposure + gamma
    col *= max(u_exposure, 0.1);
    col = sqrt(col);
    fragColor = vec4(col, 1.0);
}

)GLSL";
