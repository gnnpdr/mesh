#pragma once
#include <Eigen/Dense>
#include <cmath>

using Vec3f = Eigen::Vector3f;

namespace Vec3 {
    inline float distance(const Vec3f& a, const Vec3f& b) {
        return (a - b).norm();
    }
    
    inline Vec3f normalized(const Vec3f& v) {
        return v.normalized();
    }
    
    inline float dot(const Vec3f& a, const Vec3f& b) {
        return a.dot(b);
    }
    
    inline Vec3f cross(const Vec3f& a, const Vec3f& b) {
        return a.cross(b);
    }
    
    inline std::array<float, 4> get_plane(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2) {
        Vec3f normal = (v1 - v0).cross(v2 - v0).normalized();
        float d = -normal.dot(v0);
        return {normal.x(), normal.y(), normal.z(), d};
    }
}

inline Eigen::Vector3f toEigen(const Vec3f& v) 
{
    return Eigen::Vector3f(v.x(), v.y(), v.z());
}

inline bool has_nan(const Eigen::Matrix4f& Q) {
    return Q.array().isNaN().any();
}

inline bool is_degenerate(const Eigen::Matrix4f& Q) {
    Eigen::Matrix3f A = Q.topLeftCorner<3,3>();
    return std::abs(A.determinant()) < 1e-8f;
}