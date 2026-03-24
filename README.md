# LittleGameTest

🎮 A small native C++ game project built with Win32 and GDI.

Right now this repo contains a custom Snake game with classic rules, a desktop windowed build, and a stylized top-down 3D-like presentation.

## Features

- Classic Snake movement, food, growth, score, and game over
- Pause and restart support
- Win32 desktop window
- Custom rendering with decorative environment details
- Deterministic game logic separated from rendering

## Stack

- C++
- Win32 API
- GDI
- Visual Studio 2022

## Run

1. Open `aitest.sln` in Visual Studio 2022.
2. Build the `aitest` project with a target like `Debug | x64`.
3. Run the project.

## Controls

- `WASD` or arrow keys: move
- `P`: pause
- `R`: restart

## PR Demo

This short section is added from a demo feature branch to show how a pull request looks in practice.

## Structure

```text
.
├─ aitest.sln
├─ README.md
└─ aitest/
   ├─ main.cpp
   ├─ snake_game.cpp
   ├─ snake_game.h
   ├─ aitest.vcxproj
   └─ aitest.vcxproj.filters
```

## Notes

- [`aitest/main.cpp`](./aitest/main.cpp): Win32 app loop, input, and rendering
- [`aitest/snake_game.h`](./aitest/snake_game.h): game-state types
- [`aitest/snake_game.cpp`](./aitest/snake_game.cpp): movement, collisions, growth, and food placement

---

Built as a lightweight practice project for exploring gameplay logic and custom native rendering in C++.
