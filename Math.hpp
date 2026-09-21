#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace orbit {

struct Vec2 {
    float x = 0.0F;
    float y = 0.0F;

    constexpr Vec2() = default;
    constexpr Vec2(float xValue, float yValue) : x(xValue), y(yValue) {}

    constexpr Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    constexpr Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    constexpr Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    constexpr Vec2 operator/(float scalar) const { return {x / scalar, y / scalar}; }
    constexpr Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    constexpr Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
    constexpr Vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
};

inline constexpr Vec2 operator*(float scalar, const Vec2& vector) { return vector * scalar; }
inline constexpr float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
inline float length(const Vec2& value) { return std::sqrt(dot(value, value)); }
inline Vec2 normalized(const Vec2& value) {
    const float valueLength = length(value);
    return valueLength > 0.0001F ? value / valueLength : Vec2{};
}
inline constexpr float clamp(float value, float minimum, float maximum) {
    return std::max(minimum, std::min(value, maximum));
}

struct Rect {
    Vec2 position{};
    Vec2 size{};
};

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;

    constexpr std::uint32_t packed() const {
        return (static_cast<std::uint32_t>(r) << 16U)
             | (static_cast<std::uint32_t>(g) << 8U)
             | static_cast<std::uint32_t>(b);
    }
};

namespace colors {
inline constexpr Color navy{8, 18, 33};
inline constexpr Color ink{14, 31, 50};
inline constexpr Color white{239, 247, 255};
inline constexpr Color cyan{75, 213, 238};
inline constexpr Color coral{255, 113, 91};
inline constexpr Color blue{75, 145, 255};
inline constexpr Color red{255, 83, 83};
inline constexpr Color gold{255, 200, 87};
inline constexpr Color slate{95, 126, 150};
} // namespace colors

} // namespace orbit
