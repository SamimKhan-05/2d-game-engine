#pragma once

#include <string>
#include <windows.h>

#include "Input.hpp"
#include "Renderer.hpp"

namespace orbit {

struct ApplicationConfig {
    std::string title = "Orbit2D";
    int width = 960;
    int height = 540;
    int targetFramesPerSecond = 120;
};

class IGame {
public:
    virtual ~IGame() = default;
    virtual void onStart() {}
    virtual void onUpdate(float deltaSeconds, const Input& input) = 0;
    virtual void onRender(Renderer& renderer) = 0;
    virtual void onShutdown() {}
};

class Application {
public:
    int run(const ApplicationConfig& config, IGame& game);

private:
    static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleWindowMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    static bool mapVirtualKey(WPARAM virtualKey, Key& key);

    HWND window_ = nullptr;
    Renderer renderer_;
    Input input_;
    bool running_ = false;
};

} // namespace orbit
