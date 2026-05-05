#pragma once

#include <cmath>
#include <iostream>
#include <cassert>
#include <vector>

namespace Vec3
{

namespace Detail
{
    template<typename T>
    constexpr T EPSILON = T(1e-6);
}

/**
 * @brief Three-dimensional vector with basic mathematical operations
 * 
 * The class provides basic operations for working with 3D vectors:
 * - Addition, subtraction
 * - Operations with a scalar
 * - Dot and vector products
 * - Normalization and length calculation
 * 
 * @tparam T The data type of the vector components (usually float)
 */
template<typename T>
class Vec3
{
    T x_, y_, z_;   ///< Vector components

public:

    /**
     * @brief Default constructor. Creates a null vector (0,0,0)
     */
    Vec3() = default;

    /**
     * @brief Parameter constructor. Creates vector with given components
     * 
     * @param x Сomponent X
     * @param y Сomponent Y
     * @param z Сomponent Z
     */
    Vec3(T x, T y, T z) : x_(x), y_(y), z_(z) {}

    /**
     * @brief Retirns x component value
     * 
     * @return x component value
     */
    T x() const { return x_; }

    /**
     * @brief Retirns y component value
     * 
     * @return y component value
     */
    T y() const { return y_; }

    /**
     * @brief Retirns z component value
     * 
     * @return z component value
     */
    T z() const { return z_; }

    Vec3 operator+(const Vec3& other) const
    {
        return Vec3(x_ + other.x_, y_ + other.y_, z_ + other.z_);
    }

    Vec3 operator-(const Vec3& other) const
    {
        return Vec3(x_ - other.x_, y_ - other.y_, z_ - other.z_);
    }

    template<typename U>
    Vec3 operator*(U scalar) const 
    {
        return Vec3(x_ * static_cast<T>(scalar), y_ * static_cast<T>(scalar), z_ * static_cast<T>(scalar));
    }

    template<typename U>
    Vec3 operator/(U scalar) const 
    {
        if (!scalar)
            throw std::runtime_error("Division by zero in Vec3::operator/");
        
        return Vec3(x_ / static_cast<T>(scalar), y_ / static_cast<T>(scalar), z_ / static_cast<T>(scalar));
    }

    T dot(const Vec3& other) const
    {
        return x_ * other.x_ + y_ * other.y_ + z_ * other.z_;
    }

    Vec3 cross(const Vec3& other) const 
    {
        return Vec3(y_ * other.z_ - z_ * other.y_, z_ * other.x_ - x_ * other.z_, x_ * other.y_ - y_ * other.x_);
    }

    T length() const 
    {
        return std::sqrt(x_ * x_ + y_ * y_ + z_ * z_);
    }
    
    T squared_length() const 
    {
        return x_ * x_ + y_ * y_ + z_ * z_;
    }
    
    Vec3 normalize() const 
    {
        T len = length();
        if (len > T(0))
            return *this * (T(1) / len);
    
        return Vec3(0, 0, 0);
    }

    /**
     * @brief Сompares Vec3 by Detail::EPSILON
     * 
     * @return true if equal
     */
    bool operator==(const Vec3& other) const 
    {
        return std::abs(x_ - other.x_) < Detail::EPSILON<T> &&
               std::abs(y_ - other.y_) < Detail::EPSILON<T> &&
               std::abs(z_ - other.z_) < Detail::EPSILON<T>;
    }
    
    bool operator!=(const Vec3& other) const 
    {
        return !(*this == other);
    }

    /**
     * @brief Сhecks local proximity of points (as operator== by Detail::EPSILON, but it can be changed)
     * 
     * @return true if enough close
     */
    bool equals(const Vec3& other) const 
    {
        return std::abs(x_ - other.x_) < Detail::EPSILON<T> && std::abs(y_ - other.y_) < Detail::EPSILON<T> && std::abs(z_ - other.z_) < Detail::EPSILON<T>;
    }

    T& operator[](size_t i) 
    {
        if (i >= 3)
            throw std::out_of_range("Vec3 index out of range: " + std::to_string(i));
        
        return (&x_)[i];
    }
    
    const T& operator[](size_t i) const 
    {
        if (i >= 3)
            throw std::out_of_range("Vec3 index out of range: " + std::to_string(i));

        return (&x_)[i];
    }

};

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;

template<typename T, typename U>
Vec3<T> operator*(U scalar, const Vec3<T>& v) 
{
    return v * scalar;
}

template<typename T>
T distance(const Vec3<T>& a, const Vec3<T>& b) 
{
    return (a - b).length();
}

template<typename T>
T distance_squared(const Vec3<T>& a, const Vec3<T>& b) 
{
    return (a - b).squared_length();
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const Vec3<T>& v) 
{
    os << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
    return os;
}

inline Vec3f get_normal(Vec3f dot1, Vec3f dot2, Vec3f dot3)
{
    return (dot2 - dot1).cross(dot3 - dot1).normalize();
}

inline std::vector<float> get_plane(Vec3f dot1, Vec3f dot2, Vec3f dot3)
{
    Vec3f normal = get_normal(dot1, dot2, dot3);

    float d = -normal.dot(dot1);

    return {normal.x(), normal.y(), normal.z(), d};
}

}