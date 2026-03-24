# LittleGameTest

🎮 A lightweight native C++ game project built with Win32 and GDI.

The repository currently features a custom Snake game implemented without an external engine, focused on classic gameplay rules plus a more stylized desktop presentation.

## ✨ Highlights

- Classic Snake gameplay loop
- Native Win32 desktop window
- Custom-rendered scene with a top-down 3D-style look
- Deterministic game logic separated from rendering
- No external engine or third-party runtime dependencies

## 🐍 Current Game

The current playable build includes:
- Grid-based snake movement
- Food spawning
- Snake growth
- Score tracking
- Game-over detection
- Pause and restart support
- Decorative environment details such as grass and stones

The environment props are visual only and never affect gameplay or collision logic.

## 🛠 Tech Stack

- C++
- Win32 API
- GDI
- Visual Studio 2022 solution/project files

## 📁 Project Structure

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

## ▶️ Run Locally

1. Open `aitest.sln` in Visual Studio 2022.
2. Select a build target such as `Debug | x64`.
3. Build and run the `aitest` project.

## 🎯 Controls

- `W`, `A`, `S`, `D`: Move
- Arrow keys: Move
- `P`: Pause / resume
- `R`: Restart

## 🧠 Code Layout

- [`aitest/main.cpp`](./aitest/main.cpp)
  Win32 window creation, message loop, rendering, input, and scene presentation
- [`aitest/snake_game.h`](./aitest/snake_game.h)
  Core game-state definitions
- [`aitest/snake_game.cpp`](./aitest/snake_game.cpp)
  Deterministic snake logic, movement, growth, collisions, and food placement

The rule layer is kept separate from the rendering layer so the visual style can continue evolving without rewriting the core game logic.

## ✅ Manual Verification

- Verify `WASD` and arrow keys both control the snake correctly.
- Verify quick direction changes feel responsive.
- Verify food increases score and snake length.
- Verify wall collisions and self-collisions trigger game over.
- Verify `P` pauses the game and `R` restarts it.
- Verify decorative map objects never block gameplay.

## 🚧 Current Scope

Implemented:
- Single playable Snake game
- Native desktop rendering
- Stylized visual scene
- Local project structure for iterative development

Not included yet:
- Audio
- Menu system
- Difficulty selection
- Multiple game modes
- Save/load

## 🌱 Future Ideas

- Improve the snake model and turn smoothing
- Add richer environment variety
- Add sound effects and UI polish
- Add a title screen and settings
- Expand the repo into a small collection of native mini-games

---

Built as a small native game-dev practice project for experimenting with gameplay architecture, rendering, and iteration inside a minimal C++ codebase.
