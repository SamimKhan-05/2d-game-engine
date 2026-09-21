#pragma once

#include <array>
#include <cstddef>

namespace orbit {

enum class Key : std::size_t {
    up,
    down,
    left,
    right,
    w,
    a,
    s,
    d,
    r,
    escape,
    space,
    q,
    e,
    i,
    j,
    k,
    l,
    u,
    o,
    n,
    m,
    one,
    two,
    three,
    four,
    five,
    seven,
    eight,
    nine,
    count
};

inline constexpr std::size_t kKeyCount = static_cast<std::size_t>(Key::count);

class Input {
public:
    void beginFrame() { previous_ = current_; }
    void set(Key key, bool isDown) { current_[toIndex(key)] = isDown; }

    [[nodiscard]] bool down(Key key) const { return current_[toIndex(key)]; }
    [[nodiscard]] bool pressed(Key key) const {
        const std::size_t index = toIndex(key);
        return current_[index] && !previous_[index];
    }
    [[nodiscard]] bool released(Key key) const {
        const std::size_t index = toIndex(key);
        return !current_[index] && previous_[index];
    }

private:
    static constexpr std::size_t toIndex(Key key) { return static_cast<std::size_t>(key); }
    std::array<bool, kKeyCount> current_{};
    std::array<bool, kKeyCount> previous_{};
};

} // namespace orbit
