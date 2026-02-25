#pragma once

#include <cmath>
#include <iostream>
#include <cassert>

namespace Vec3
{

namespace Detail
{
    template<typename T>
    constexpr T EPSILON = T(1e-6);
}


template<typename T>
class Vec3
{
    T x_, y_, z_;

public:

    Vec3() = default;
    Vec3(T x, T y, T z) : x_(x), y_(y), z_(z) {}

    T x() const { return x_; }
    T y() const { return y_; }
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
    
    T squaredLength() const 
    {
        return x_ * x_ + y_ * y_ + z_ * z_;
    }
    
    Vec3 normalized() const 
    {
        T len = length();
        if (len > T(0))
            return *this * (T(1) / len);
    
        return Vec3(0, 0, 0);
    }
    
    //может возникнуть проблема
    bool operator==(const Vec3& other) const 
    {
        return x_ == other.x_ && y_ == other.y_ && z_ == other.z_;
    }
    
    bool operator!=(const Vec3& other) const 
    {
        return !(*this == other);
    }

    //проверка локальной близости точек 
    bool equals(const Vec3& other) const {
        return std::abs(x_ - other.x_) < Detail::EPSILON<T> && std::abs(y_ - other.y_) < Detail::EPSILON<T> && std::abs(z_ - other.z_) < Detail::EPSILON<T>;
    }

    T& operator[](size_t i) 
    {
        assert(i < 3); 
        return (&x_)[i];
    }
    
    const T& operator[](size_t i) const 
    {
        assert(i < 3); 
        return (&x_)[i];
    }

};

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
T distanceSquared(const Vec3<T>& a, const Vec3<T>& b) 
{
    return (a - b).squaredLength();
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const Vec3<T>& v) 
{
    os << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
    return os;
}

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;
}