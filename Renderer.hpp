#pragma once

#include <string_view>
#include <vector>
#include <windows.h>

#include "Math.hpp"

namespace orbit {

class Renderer {
public:
    void resize(int width, int height);

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }

    void clear(Color color);
    void fillRect(Rect rectangle, Color color);
    void drawRect(Rect rectangle, Color color, int thickness = 1);
    void drawLine(Vec2 start, Vec2 end, Color color, int thickness = 1);
    void fillCircle(Vec2 center, float radius, Color color);
    void drawCircle(Vec2 center, float radius, Color color, int thickness = 1);
    void drawText(Vec2 position, std::string_view text, Color color, int scale = 1);
    void present(HDC deviceContext) const;

private:
    void setPixel(int x, int y, Color color);
    void drawGlyph(int x, int y, char character, Color color, int scale);

    int width_ = 0;
    int height_ = 0;
    std::vector<std::uint32_t> pixels_;
};

} // namespace orbit
