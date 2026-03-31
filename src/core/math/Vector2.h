#pragma once

namespace fm::core {

struct Vector2 {
    double x {0.0};
    double y {0.0};

    Vector2& operator+=(const Vector2& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
};

inline Vector2 operator+(Vector2 lhs, const Vector2& rhs)
{
    lhs += rhs;
    return lhs;
}

inline Vector2 operator*(const Vector2& value, double scalar)
{
    return {value.x * scalar, value.y * scalar};
}

}  // namespace fm::core