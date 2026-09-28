#pragma once
#include <cmath>
#include <algorithm>

struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator*(const Vec3& o) const { return {x * o.x, y * o.y, z * o.z}; } // component-wise
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    Vec3 operator-() const { return {-x, -y, -z}; }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float length2() const { return x * x + y * y + z * z; }
    Vec3 normalized() const {
        float len = length();
        return len > 1e-8f ? *this / len : Vec3(0, 0, 0);
    }
    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    Vec3 reflect(const Vec3& n) const {
        return *this - n * (2.0f * this->dot(n));
    }
    Vec3 clamp01() const {
        return {std::max(0.f, std::min(1.f, x)),
                std::max(0.f, std::min(1.f, y)),
                std::max(0.f, std::min(1.f, z))};
    }
};

inline Vec3 operator*(float s, const Vec3& v) { return v * s; }
