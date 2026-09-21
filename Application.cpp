#include "Application.hpp"

#include <algorithm>
#include <chrono>
#include <thread>

namespace orbit {

namespace {
constexpr char kWindowClassName[] = "Orbit2DWindowClass";
}

int Application::run(const ApplicationConfig& config, IGame& game) {
    const HINSTANCE instance = GetModuleHandleA(nullptr);

    WNDCLASSEXA windowClass{};
    windowClass.cbSize = sizeof(WNDCLASSEXA);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &Application::windowProcedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    windowClass.lpszClassName = kWindowClassName;

    if (RegisterClassExA(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return 1;
    }

    RECT windowRect{0, 0, config.width, config.height};
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRectEx(&windowRect, style, FALSE, 0);

    window_ = CreateWindowExA(
        0,
        kWindowClassName,
        config.title.c_str(),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instance,
        this);

    if (window_ == nullptr) {
        return 1;
    }

    renderer_.resize(config.width, config.height);
    ShowWindow(window_, SW_SHOWDEFAULT);
    UpdateWindow(window_);

    running_ = true;
    game.onStart();
    auto previousFrameTime = std::chrono::steady_clock::now();
    const float targetFrameSeconds = config.targetFramesPerSecond > 0
        ? 1.0F / static_cast<float>(config.targetFramesPerSecond)
        : 0.0F;

    while (running_) {
        const auto frameStart = std::chrono::steady_clock::now();
        input_.beginFrame();

        MSG message{};
        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                running_ = false;
                break;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        const auto currentFrameTime = std::chrono::steady_clock::now();
        const float deltaSeconds = std::min(
            std::chrono::duration<float>(currentFrameTime - previousFrameTime).count(),
            1.0F / 30.0F);
        previousFrameTime = currentFrameTime;

        if (running_) {
            game.onUpdate(deltaSeconds, input_);
            game.onRender(renderer_);

            HDC deviceContext = GetDC(window_);
            renderer_.present(deviceContext);
            ReleaseDC(window_, deviceContext);
        }

        if (targetFrameSeconds > 0.0F) {
            const float frameSeconds = std::chrono::duration<float>(
                std::chrono::steady_clock::now() - frameStart).count();
            if (frameSeconds < targetFrameSeconds) {
                std::this_thread::sleep_for(std::chrono::duration<float>(targetFrameSeconds - frameSeconds));
            }
        }
    }

    game.onShutdown();
    if (window_ != nullptr && IsWindow(window_)) {
        DestroyWindow(window_);
    }
    window_ = nullptr;
    return 0;
}

LRESULT CALLBACK Application::windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTA*>(lParam);
        auto* application = static_cast<Application*>(create->lpCreateParams);
        SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(application));
        application->window_ = window;
    }

    auto* application = reinterpret_cast<Application*>(GetWindowLongPtrA(window, GWLP_USERDATA));
    return application != nullptr
        ? application->handleWindowMessage(window, message, wParam, lParam)
        : DefWindowProcA(window, message, wParam, lParam);
}

LRESULT Application::handleWindowMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        running_ = false;
        PostQuitMessage(0);
        return 0;
    case WM_SIZE: {
        const int width = LOWORD(lParam);
        const int height = HIWORD(lParam);
        if (width > 0 && height > 0) {
            renderer_.resize(width, height);
        }
        return 0;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        Key key{};
        if (mapVirtualKey(wParam, key)) {
            input_.set(key, message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
        }
        if (wParam == VK_ESCAPE && (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)) {
            DestroyWindow(window);
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC deviceContext = BeginPaint(window, &paint);
        renderer_.present(deviceContext);
        EndPaint(window, &paint);
        return 0;
    }
    default:
        return DefWindowProcA(window, message, wParam, lParam);
    }
}

bool Application::mapVirtualKey(WPARAM virtualKey, Key& key) {
    switch (virtualKey) {
    case VK_UP: key = Key::up; return true;
    case VK_DOWN: key = Key::down; return true;
    case VK_LEFT: key = Key::left; return true;
    case VK_RIGHT: key = Key::right; return true;
    case 'W': key = Key::w; return true;
    case 'A': key = Key::a; return true;
    case 'S': key = Key::s; return true;
    case 'D': key = Key::d; return true;
    case 'R': key = Key::r; return true;
    case 'Q': key = Key::q; return true;
    case 'E': key = Key::e; return true;
    case 'I': key = Key::i; return true;
    case 'J': key = Key::j; return true;
    case 'K': key = Key::k; return true;
    case 'L': key = Key::l; return true;
    case 'U': key = Key::u; return true;
    case 'O': key = Key::o; return true;
    case 'N': key = Key::n; return true;
    case 'M': key = Key::m; return true;
    case '1': key = Key::one; return true;
    case '2': key = Key::two; return true;
    case '3': key = Key::three; return true;
    case '4': key = Key::four; return true;
    case '5': key = Key::five; return true;
    case '7': key = Key::seven; return true;
    case '8': key = Key::eight; return true;
    case '9': key = Key::nine; return true;
    case VK_ESCAPE: key = Key::escape; return true;
    case VK_SPACE: key = Key::space; return true;
    default: return false;
    }
}

} // namespace orbit
