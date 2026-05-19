/**
 * @file vec3.hpp
 * @brief Раньше здесь был реализован вектор, на основе коротого строился весь проект, но используется библиотека Eigen, так что на траиваем ее для совпадения интерфейса с остальным кодом 
 * 
 * @note Eigen используеем для SIMD-оптимизаций и высокой производительности
 */

#pragma once

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <array>
#include <cmath>
#include <stdexcept>

/**
 * @brief Основной тип для трёхмерных векторов
 */
using Vec3f = Eigen::Vector3f;

namespace Vec3 {
    
    inline float distance(const Vec3f& a, const Vec3f& b) { return (a - b).norm(); }
    
    inline Vec3f normalized(const Vec3f& v) 
    {
        float len = v.norm();
        if (len < 1e-9f)
            return Vec3f(0.0f, 0.0f, 0.0f);
    
        return v / len;
    }
    
    inline float dot(const Vec3f& a, const Vec3f& b) { return a.dot(b); }
    
    inline Vec3f cross(const Vec3f& a, const Vec3f& b) { return a.cross(b); }
    
    /**
     * @brief Вычисляет уравнение плоскости (a*x + b*y + c*z + d = 0) по трём точкам
     * @throws std::runtime_error Если точки коллинеарны (площадь треугольника слишком мала)
     */
    inline std::array<float, 4> get_plane(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2) 
    {
        Vec3f edge1 = v1 - v0;
        Vec3f edge2 = v2 - v0;
        Vec3f normal = edge1.cross(edge2);
        
        float len = normal.norm();
        if (len < 1e-9f) {
            throw std::runtime_error("Vec3::get_plane: degenerate triangle (points are collinear)");
        }
        
        normal /= len;
        float d = -normal.dot(v0);
        
        return {normal.x(), normal.y(), normal.z(), d};
    }
    
    inline Vec3f get_normal(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2) {
        Vec3f normal = (v1 - v0).cross(v2 - v0);
        float len = normal.norm();
        if (len < 1e-9f) {
            throw std::runtime_error("Vec3::get_normal: degenerate triangle");
        }
        return normal / len;
    }
    
} 
/**
 * @brief Для совместимости. Преобразует Vec3f в Eigen::Vector3f
 */
inline Eigen::Vector3f toEigen(const Vec3f& v)  { return Eigen::Vector3f(v.x(), v.y(), v.z()); }

/**
 * @brief Проверяет наличие nan значений в матрице 4x4
 */
inline bool has_nan(const Eigen::Matrix4f& Q) { return Q.array().isNaN().any(); }

/**
 * @brief Проверяет вырожденность матрицы 4x4
 */
inline bool is_degenerate(const Eigen::Matrix4f& Q) 
{
    Eigen::Matrix3f A = Q.topLeftCorner<3,3>();
    return std::abs(A.determinant()) < 1e-8f;
}

inline std::ostream& operator<<(std::ostream& os, const Vec3f& v) 
{
    os << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
    return os;
}