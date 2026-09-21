# Orbit2D

Orbit2D is a compact C++20 2D engine for Windows. It deliberately uses Win32 and GDI only, so the repository can be compiled without downloading a framework. It includes two playable samples: the Pong arena and the Arena Clash fighting game.

## Included systems

- Win32 application loop with fixed client resolution, delta time, frame cap, and clean shutdown.
- Keyboard input with held, pressed, and released state.
- Software framebuffer renderer: rectangles, lines, circles, pixel-font text, and colors.
- Component-style scene entities with transform, sprite, rigid body, and AABB collider components.
- 2D camera transform and impulse-based static/dynamic AABB collision resolution.
- Trigger colliders and collision event reporting.

## Build

### CMake

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\bin\orbit2d.exe
.\build\bin\arena_clash.exe
```

### MSYS2 UCRT64 g++

```powershell
New-Item -ItemType Directory -Force build | Out-Null
& C:\msys64\ucrt64\bin\g++.exe -std=c++20 -Wall -Wextra -pedantic -mwindows src\main.cpp src\engine\Application.cpp src\engine\Renderer.cpp src\engine\Scene.cpp -o build\orbit2d.exe -luser32 -lgdi32
.\build\orbit2d.exe
```

To build Arena Clash directly with MSYS2 UCRT64 g++:

```powershell
& C:\msys64\ucrt64\bin\g++.exe -std=c++20 -Wall -Wextra -pedantic -mwindows src\fighting_game.cpp src\engine\Application.cpp src\engine\Renderer.cpp -o build\arena_clash.exe -luser32 -lgdi32
.\build\arena_clash.exe
```

## Pong controls

- `W` / `S`: move the left paddle.
- `Up` / `Down`: move the right paddle.
- `R`: reset the match.
- `Esc`: close the window.

## Pong match rules

- Each round starts with a three-second `3`, `2`, `1` countdown, including after every score.
- The round number is shown in the HUD and on the countdown panel.
- Blue and red compete to be the first to 10 points; the winner receives a color-matched victory screen.
- Ball speed rises by 24 on each paddle hit, up to 680. It resets to 410 when the next round begins.

## Arena Clash controls

Arena Clash is a local two-player side-on fighter with health bars, weapon reach/damage differences, shields, arcing grenades, jump, dash, and animated combat visuals.

- Blue: `A` / `D` move, `W` jump, `S` dash, `Q` attack, `E` use offhand. Select `1` sword, `2` axe, `3` spear, `4` grenade, or `5` shield.
- Red: `J` / `L` move, `I` jump, `K` dash, `U` attack, `O` use offhand. Select `7` sword, `8` axe, `9` spear, `N` grenade, or `M` shield.
- Shield blocks incoming damage while active. Grenades arc, bounce, then explode. Press `R` to reset after a victory.
- The arena, fighters, weapon swings, shield pulses, grenade spin, explosions, health bars, cooldown meters, and victory screen animate continuously.

## Creating a game

Implement `orbit::IGame`, then pass it to `orbit::Application::run`. `PongGame` in `src/main.cpp` shows the intended flow: spawn scene entities in `onStart`, apply gameplay rules and call `Scene::step` from `onUpdate`, and render with `Scene::render` from `onRender`.
