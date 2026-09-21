#include "Renderer.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>

namespace orbit {

namespace {
using Glyph = std::array<std::uint8_t, 7>;

Glyph glyphFor(char character) {
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(character)))) {
    case 'A': return {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
    case 'B': return {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110};
    case 'C': return {0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110};
    case 'D': return {0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110};
    case 'E': return {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111};
    case 'F': return {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000};
    case 'G': return {0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01110};
    case 'H': return {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
    case 'I': return {0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110};
    case 'J': return {0b00001, 0b00001, 0b00001, 0b00001, 0b10001, 0b10001, 0b01110};
    case 'K': return {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001};
    case 'L': return {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111};
    case 'M': return {0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001};
    case 'N': return {0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001};
    case 'O': return {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110};
    case 'P': return {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000};
    case 'Q': return {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101};
    case 'R': return {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001};
    case 'S': return {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110};
    case 'T': return {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100};
    case 'U': return {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110};
    case 'V': return {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100};
    case 'W': return {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010};
    case 'X': return {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001};
    case 'Y': return {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100};
    case 'Z': return {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111};
    case '0': return {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110};
    case '1': return {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110};
    case '2': return {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111};
    case '3': return {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110};
    case '4': return {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010};
    case '5': return {0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110};
    case '6': return {0b01110, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110};
    case '7': return {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000};
    case '8': return {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110};
    case '9': return {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110};
    case ':': return {0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b00000};
    case '-': return {0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000};
    case '.': return {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b00100};
    default: return {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000};
    }
}
} // namespace

void Renderer::resize(int width, int height) {
    width_ = std::max(width, 1);
    height_ = std::max(height, 1);
    pixels_.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), 0U);
}

void Renderer::clear(Color color) {
    std::fill(pixels_.begin(), pixels_.end(), color.packed());
}

void Renderer::setPixel(int x, int y, Color color) {
    if (x >= 0 && x < width_ && y >= 0 && y < height_) {
        pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)] = color.packed();
    }
}

void Renderer::fillRect(Rect rectangle, Color color) {
    const int left = std::max(0, static_cast<int>(std::floor(rectangle.position.x)));
    const int top = std::max(0, static_cast<int>(std::floor(rectangle.position.y)));
    const int right = std::min(width_, static_cast<int>(std::ceil(rectangle.position.x + rectangle.size.x)));
    const int bottom = std::min(height_, static_cast<int>(std::ceil(rectangle.position.y + rectangle.size.y)));

    for (int y = top; y < bottom; ++y) {
        const std::size_t rowStart = static_cast<std::size_t>(y) * static_cast<std::size_t>(width_);
        for (int x = left; x < right; ++x) {
            pixels_[rowStart + static_cast<std::size_t>(x)] = color.packed();
        }
    }
}

void Renderer::drawRect(Rect rectangle, Color color, int thickness) {
    const float lineWidth = static_cast<float>(std::max(thickness, 1));
    fillRect({rectangle.position, {rectangle.size.x, lineWidth}}, color);
    fillRect({{rectangle.position.x, rectangle.position.y + rectangle.size.y - lineWidth}, {rectangle.size.x, lineWidth}}, color);
    fillRect({rectangle.position, {lineWidth, rectangle.size.y}}, color);
    fillRect({{rectangle.position.x + rectangle.size.x - lineWidth, rectangle.position.y}, {lineWidth, rectangle.size.y}}, color);
}

void Renderer::drawLine(Vec2 start, Vec2 end, Color color, int thickness) {
    int x0 = static_cast<int>(std::lround(start.x));
    int y0 = static_cast<int>(std::lround(start.y));
    const int x1 = static_cast<int>(std::lround(end.x));
    const int y1 = static_cast<int>(std::lround(end.y));
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    const int halfThickness = std::max(thickness, 1) / 2;

    while (true) {
        fillRect({{static_cast<float>(x0 - halfThickness), static_cast<float>(y0 - halfThickness)},
                  {static_cast<float>(std::max(thickness, 1)), static_cast<float>(std::max(thickness, 1))}}, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int twiceError = 2 * error;
        if (twiceError >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twiceError <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

void Renderer::fillCircle(Vec2 center, float radius, Color color) {
    const int integerRadius = std::max(0, static_cast<int>(std::ceil(radius)));
    const int radiusSquared = integerRadius * integerRadius;
    const int centerX = static_cast<int>(std::lround(center.x));
    const int centerY = static_cast<int>(std::lround(center.y));

    for (int y = -integerRadius; y <= integerRadius; ++y) {
        for (int x = -integerRadius; x <= integerRadius; ++x) {
            if (x * x + y * y <= radiusSquared) {
                setPixel(centerX + x, centerY + y, color);
            }
        }
    }
}

void Renderer::drawCircle(Vec2 center, float radius, Color color, int thickness) {
    const int outerRadius = std::max(0, static_cast<int>(std::ceil(radius)));
    const int innerRadius = std::max(0, outerRadius - std::max(thickness, 1));
    const int outerRadiusSquared = outerRadius * outerRadius;
    const int innerRadiusSquared = innerRadius * innerRadius;
    const int centerX = static_cast<int>(std::lround(center.x));
    const int centerY = static_cast<int>(std::lround(center.y));

    for (int y = -outerRadius; y <= outerRadius; ++y) {
        for (int x = -outerRadius; x <= outerRadius; ++x) {
            const int distanceSquared = x * x + y * y;
            if (distanceSquared <= outerRadiusSquared && distanceSquared >= innerRadiusSquared) {
                setPixel(centerX + x, centerY + y, color);
            }
        }
    }
}

void Renderer::drawText(Vec2 position, std::string_view text, Color color, int scale) {
    const int glyphScale = std::max(scale, 1);
    const int originX = static_cast<int>(std::lround(position.x));
    int cursorX = originX;
    int cursorY = static_cast<int>(std::lround(position.y));

    for (const char character : text) {
        if (character == '\n') {
            cursorX = originX;
            cursorY += 8 * glyphScale;
            continue;
        }
        drawGlyph(cursorX, cursorY, character, color, glyphScale);
        cursorX += 6 * glyphScale;
    }
}

void Renderer::drawGlyph(int x, int y, char character, Color color, int scale) {
    const Glyph glyph = glyphFor(character);
    for (int row = 0; row < 7; ++row) {
        for (int column = 0; column < 5; ++column) {
            if ((glyph[static_cast<std::size_t>(row)] & (1U << (4 - column))) != 0U) {
                fillRect({{static_cast<float>(x + column * scale), static_cast<float>(y + row * scale)},
                          {static_cast<float>(scale), static_cast<float>(scale)}}, color);
            }
        }
    }
}

void Renderer::present(HDC deviceContext) const {
    if (pixels_.empty()) {
        return;
    }

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width_;
    bitmapInfo.bmiHeader.biHeight = -height_; // A negative height makes the DIB top-down.
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(deviceContext, 0, 0, width_, height_, 0, 0, width_, height_,
        pixels_.data(), &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

} // namespace orbit
