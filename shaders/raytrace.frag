// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
#version 100
// GLES2 / WebGL1 raytracer. No scene meshes — pure analytic intersections.
precision highp float;
varying vec2 v_uv;

#define MAX_SPHERES 16
#define MAX_BOXES   128
#define MAX_PLANES  12

uniform vec3 u_camPos;
uniform vec3 u_camForward;
uniform vec3 u_camRight;
uniform vec3 u_camUp;
uniform float u_fovScale;
uniform float u_aspect;
uniform float u_pixelAngle; // 2 * fovScale / render height (radians per pixel, small-angle)

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
uniform int u_planeOneSided[MAX_PLANES];

struct Hit {
    float t;
    vec3 point;
    vec3 normal;
    vec3 color;
    float reflectivity;
    float radius; // > 0 for curved surfaces (sphere), 0 for flat
    bool hit;
};

// Integral of the unit square wave (-1)^floor(x): a triangle wave, period 2.
float squareIntegral(float x) {
    float m = x - 2.0 * floor(x * 0.5);
    return 1.0 - abs(m - 1.0);
}

// Box-filtered (-1)^floor(x) over [x - w/2, x + w/2]; tends to 0 as the filter widens.
float filteredSquare(float x, float w) {
    w = max(w, 1e-4);
    return (squareIntegral(x + 0.5 * w) - squareIntegral(x - 0.5 * w)) / w;
}

// fw0 / fa: pixel footprint model (world width at distance d is fw0 + fa * d),
// used to filter the procedural checkers. See Ray in include/scene.hpp.
Hit intersect(vec3 ro, vec3 rd, float fw0, float fa) {
    Hit best;
    best.t = 1e30;
    best.hit = false;
    best.color = vec3(0.0);
    best.reflectivity = 0.0;
    best.radius = 0.0;
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
            best.radius = u_sphereRadius[i];
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
        // Face normal = axis along which the hit point is closest to the box surface.
        vec3 q = d / max(abs(halfExtent), vec3(1e-6));
        vec3 aq = abs(q);
        if (aq.x >= aq.y && aq.x >= aq.z) {
            best.normal = vec3(q.x < 0.0 ? -1.0 : 1.0, 0.0, 0.0);
        } else if (aq.y >= aq.z) {
            best.normal = vec3(0.0, q.y < 0.0 ? -1.0 : 1.0, 0.0);
        } else {
            best.normal = vec3(0.0, 0.0, q.z < 0.0 ? -1.0 : 1.0);
        }
        best.color = u_boxColor[i];
        best.reflectivity = u_boxReflect[i];
        best.radius = 0.0;
        best.hit = true;
    }

    for (int i = 0; i < MAX_PLANES; ++i) {
        if (i >= u_numPlanes) break;
        float denom = dot(u_planeNormal[i], rd);
        if (abs(denom) < 1e-6) continue;
        if (u_planeOneSided[i] != 0 && denom > 0.0) continue; // seen from behind
        float t = dot(u_planePoint[i] - ro, u_planeNormal[i]) / denom;
        if (t < 1e-4 || t >= best.t) continue;

        best.t = t;
        best.point = ro + rd * t;
        best.normal = denom < 0.0 ? u_planeNormal[i] : -u_planeNormal[i];
        best.reflectivity = u_planeReflect[i];
        best.radius = 0.0;

        if (u_planeChecker[i] != 0) {
            float u, v;   // position on the plane's tangent axes
            float du, dv; // ray direction on the same axes
            if (abs(u_planeNormal[i].y) > 0.9) {
                u = best.point.x; v = best.point.z;
                du = rd.x;        dv = rd.z;
            } else if (abs(u_planeNormal[i].x) > 0.9) {
                u = best.point.z; v = best.point.y;
                du = rd.z;        dv = rd.y;
            } else {
                u = best.point.x; v = best.point.y;
                du = rd.x;        dv = rd.y;
            }
            // Pixel footprint on the plane: an ellipse stretched by 1/cos along the
            // ray's projected direction; filter the checker over its axis-aligned extents.
            float w = fw0 + fa * t;
            float minorW = w;
            float majorW = w / max(0.05, abs(denom));
            float len = sqrt(du * du + dv * dv);
            float mu = len > 1e-6 ? du / len : 1.0;
            float mv = len > 1e-6 ? dv / len : 0.0;
            float wu = sqrt(majorW * majorW * mu * mu + minorW * minorW * mv * mv);
            float wv = sqrt(majorW * majorW * mv * mv + minorW * minorW * mu * mu);
            float sU = filteredSquare(u * u_planeScale[i], wu * u_planeScale[i]);
            float sV = filteredSquare(v * u_planeScale[i], wv * u_planeScale[i]);
            float fracA = 0.5 - 0.5 * sU * sV; // 1 = fully colorA
            best.color = mix(u_planeColorB[i], u_planeColorA[i], fracA);
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
        Hit sh = intersect(p + n * 1e-3, toL, 0.0, 0.0);
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
// Mirrors CpuRaytracer::shade: a hit blends local shading with the reflection
// unless it is the last allowed bounce, where local shading is used in full.
vec3 trace(vec3 ro, vec3 rd) {
    vec3 throughput = vec3(1.0);
    vec3 result = vec3(0.0);
    float fw0 = 0.0;
    float fa = u_pixelAngle;

    int maxB = clamp(u_maxBounces, 0, 3);
    for (int bounce = 0; bounce <= 3; ++bounce) {
        if (bounce > maxB) {
            break;
        }
        Hit h = intersect(ro, rd, fw0, fa);
        if (!h.hit) {
            result += throughput * u_skyColor;
            break;
        }

        vec3 local = shadeHit(h, rd);
        float kr = h.reflectivity;
        bool reflects = kr >= 0.01 && bounce < maxB;
        result += throughput * local * (reflects ? (1.0 - kr) : 1.0);

        if (!reflects) {
            break;
        }

        // Continue along the reflection, carrying the pixel footprint.
        float wHit = fw0 + fa * h.t;
        fw0 = wHit;
        fa += h.radius > 0.0 ? 2.0 * wHit / h.radius : 0.0;
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
    gl_FragColor = vec4(col, 1.0);
}
