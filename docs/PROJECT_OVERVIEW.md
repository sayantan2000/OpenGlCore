# Project Overview

## Purpose
OpenGlCore is a **personal learning project** for modern OpenGL (3.3 core profile) on
Windows. The goal is to understand the graphics pipeline hands-on — buffers, shaders,
cameras, lighting, texturing — by building thin C++ wrappers over raw GL calls rather
than using a framework.

## Current state (as of 2026-11-19)
A single executable that:
- Creates a **fullscreen** GLFW window sized to the primary monitor.
- Loads GLAD, enables depth testing.
- Draws one **textured, Phong-lit pyramid** (`default.vert`/`default.frag`) that slowly
  rotates.
- Attempts to draw a **light-source sphere** (`Light.vert`/`Light.frag`) — currently
  broken (see Roadmap: sphere generation + switch fall-through).
- Provides a **WASD + mouse-look fly camera** (hold left mouse to look).
- Shows **FPS / frame-time in the window title**.

### What works
- Window + context creation, GLAD load, depth test.
- Pyramid: geometry, texture (`Brick.jpg`), ambient+diffuse+specular lighting.
- Camera movement and look.
- Shader compile/link with error logging.

### What is stubbed or broken
- **Sphere / light source** — geometry generation is incorrect (see `docs/ROADMAP.md`).
- **`Text` class** — declared, fully empty (intended freetype text rendering).
- **`Model` class** — empty `Load()` stub; `Models/Cube.fbx` present but no loader.
- **freetype + zlib** — vendored under `Libraries/source` but **not referenced** by any
  project code.

## Scope / non-goals
- Not a general-purpose engine, editor, or cross-platform project (Windows-only;
  uses `Windows.h`, `GetSystemMetrics`).
- No asset pipeline, no scene graph, no serialization.

## Documentation map
- `docs/ARCHITECTURE.md` — modules, dependencies, data flow.
- `docs/RENDERING_PIPELINE.md` — per-frame render flow and shaders.
- `docs/RESOURCE_MANAGEMENT.md` — GPU resource ownership & lifecycle.
- `docs/ROADMAP.md` — prioritized refactoring plan (requires approval to execute).
- `docs/DECISIONS.md` — architectural decisions and rationale.
- `CLAUDE.md` — agent quick-start and hard rules.
