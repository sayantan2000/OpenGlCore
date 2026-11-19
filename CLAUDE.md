# CLAUDE.md — Agent Instructions for OpenGlCore

> Read this first. For deeper context see the `docs/` folder. Do **not** re-analyze the
> whole codebase each session — the docs below already capture the current state.

## What this project is
A **learning-oriented OpenGL 3.3 renderer** (Windows / Visual Studio 2022, C++20).
It opens a fullscreen window, draws a textured, lit pyramid and an (intended) light
sphere, with a fly-through camera and an FPS counter in the title bar. It is **not** an
engine yet — it is a single `main.cpp` demo backed by thin RAII-less wrapper classes.

## Build & run
- Open `OpenGlCore.sln` in Visual Studio 2022, build **Debug|x64** (or Release), run.
- Include path: `OpenGlCore/Libraries/source`; lib path: `OpenGlCore/Libraries/lib`.
- Links `glfw3.lib` + `opengl32.lib`. GLAD (`glad.c`) and stb (`stb.cpp`) are compiled in.
- **Working directory matters**: shaders/textures are loaded via relative paths
  (`./Shader/...`, `./Images/...`). Run with the project dir as CWD or paths will fail.
- `.vscode/tasks.json` is a stale auto-generated single-file clang task — **ignore it**;
  it does not build the project. MSBuild/VS is the real build.

## Layout
- `OpenGlCore/*.cpp,*.h` — the actual project source (see `docs/ARCHITECTURE.md`).
- `OpenGlCore/Shader/` — GLSL (`default.*` = lit object, `Light.*` = light source).
- `OpenGlCore/Images/`, `Fonts/`, `Models/` — assets.
- `OpenGlCore/Libraries/source/` — **vendored** GLFW, GLAD, GLM, stb, freetype, zlib.
  Treat as third-party; do not edit or document.

## Conventions (current, inconsistent — preserve unless refactoring)
- Class names mix casing oddly: `VerTexArray`, `VerTexBuffer`, `Triangle` (= an EBO,
  misleading name), `SeUniFormValueMattrix` (typos are load-bearing — callers depend on
  the exact spelling). Do not rename without updating every call site.
- Vertex layout for lit objects is **11 floats**: pos(3) color(3) uv(2) normal(3).

## Hard rules for agents
- **Do not refactor source code without explicit approval.** See `docs/ROADMAP.md` for
  the agreed plan and priorities; implement only approved items.
- Preserve existing runtime behavior; prefer small, testable changes.
- Do not add dependencies without justification (freetype + zlib are already vendored
  but **currently unused** — see roadmap before wiring them in).
- Build after changes; there is no test suite, so verify by running the app.

## Status
- **Phase 0 (critical bugs) + Phase 1.1 (RAII GL wrappers) done** — see `docs/ROADMAP.md`.
  Debug|x64 builds. GL wrappers are now RAII: non-copyable, movable; freed in destructors
  inside a scope block in `main` (before `glfwTerminate`).

## Known-broken / sharp edges (details in docs/ROADMAP.md)
- No GL error checking around buffer/texture creation; missing shader file does
  `throw(errno)` → abort (Phase 1.2, pending).
- `Triangle` means EBO; `VerticesStride()`/`TriangleStride()` return bytes (Phase 2.1).
- Sphere is position-only (light marker); no normals/UVs if you want it lit (Phase 0.2 note).

## Timing note
Both pyramid rotation (`main.cpp`) and camera movement (`Camera.cpp`) are now delta-time
scaled. `rotationSpeedDegPerSec` in main.cpp and `Camera::speed` (units/sec) control them.
