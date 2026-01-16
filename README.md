# Ping Pong

A modern arcade-style Pong game built with legacy OpenGL and WinAPI.

## Features

- Two gameplay modes:
  - Player vs Player (local multiplayer)
  - Player vs Computer (with adaptive AI)
- Three ball types with visual effects (fire, ice, magnetic)
- 7 power-ups:
  - Big Paddle
  - Slow Ball
  - Extra Points
  - Slow Time
  - Fast Paddle
  - Invisible Ball
  - Split Ball
- Combo system with score multiplier
- Particle effects and ball trail
- 7 unlockable achievements
- Two difficulty levels (Medium / Hard)
- Adjustable ball speed in PvP mode
- Fullscreen toggle (F11)
- Responsive window resizing
- Basic sound effects

## Controls

**In Menu**  
1 — Player vs Player  
2 — Player vs Computer  
3 — Ball speed selection (PvP)  
Space — Start / Pause  
Esc — Exit / Back

**In Game**  
**Player 1 (bottom paddle)**  
A / D           — move (Keyboard mode 1)  
← / →           — move (Keyboard mode 2)  
Mouse movement  — if mouse control selected

**Player 2 (top paddle)**  
← / → or ↑ / ↓  — arrows  
Mouse (upper half of screen) — if mouse control selected

**General**  
M     — Return to menu  
R     — Restart game  
Space — Pause / Resume  
F11   — Toggle fullscreen

## Building (MSYS2 / MinGW-w64)

1. Install MSYS2: https://www.msys2.org/
2. Open **mingw64.exe** terminal
3. Update the system:

```bash
1. pacman -Syu (write Y on all questions)
2. pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain
3. gcc pingpong.c -o pingpong.exe -lopengl32 -lglu32 -lgdi32 -mwindows
4. Run game ./pingpong.exe
