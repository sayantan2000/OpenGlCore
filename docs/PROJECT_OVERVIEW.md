# Project Overview

## Purpose
OpenGlCore is a **learning project** for modern real-time rendering in OpenGL 3.3 core
(Windows / VS2022 / C++20). It began as a tutorial-style demo and now implements a
**physically based rendering (PBR) pipeline** with image-based lighting, built from small
RAII modules rather than a framework.

## Current state (as of 2026-11-20)
A single executable that renders an editable PBR scene inside a Unity-style editor:
- Default scene: a **textured wooden chair** (BaseColor/Normal/Roughness/AO maps), two
  spheres (metal + dielectric), and directional/point/spot/**rectangle (area)** lights, all
  as `GameObject`s, shaded with a **Cook-Torrance metallic-roughness** BRDF.
- **Lights**: directional, point (inverse-square), spot (inner/outer cone), and a rectangle
  area light (representative-point approximation, not full LTC).
- **Image-Based Lighting**: diffuse irradiance cubemap, prefiltered specular cubemap, and a
  BRDF LUT, generated at startup from an **equirectangular HDRI** (`.exr`/`.hdr`) — or a
  procedural sky fallback — and reused for the skybox.
- **HDR pipeline**: `RGBA16F` framebuffer → fullscreen ACES tonemap + gamma.
- **Interactive editor** (Dear ImGui + ImGuizmo): fixed Hierarchy/Inspector/Console layout
  with a central viewport; left-click ray-picking; W/E/R transform gizmos; F focus;
  Delete/Ctrl+D/Ctrl+Z/Ctrl+Y; Hierarchy Create menu (empty / 7 primitives / 4 light types);
  centralized undo/redo. The Inspector edits live scene data with immediate effect.
- An on-screen **FPS HUD** (FreeType) and a **right-mouse flythrough camera** (WASD + Shift
  sprint, Space/Ctrl up/down, mouse look while RMB held).

### What works (builds + runtime smoke test clean)
Window/context, PBR shading, all four light types, IBL generation + sampling, HDR tonemap,
model loading, text, and editor startup. The app runs without shader-compile or GL errors.
Editor **interaction/visuals are not verified** here (no display).

> Visual correctness has **not** been verified in the dev environment (no display). Run
> `./build.ps1 -Run` to view.

### Not yet done
- PBR **texture-map assets** (albedo/normal/metallic/roughness/AO) — the shader supports
  them (units 3–7) but the scene uses scalar materials; no map set is shipped.
- IBL from a **loaded equirectangular `.hdr`** (currently procedural sky).
- Framebuffer **resize** handling.
- Model **PBR map import** (diffuse color/texture only).

## Scope / non-goals
Windows-only build (VS/MSBuild); no scene graph, serialization, or asset pipeline. The
source is platform-neutral (GLFW/GLAD/GLM/Assimp/FreeType/stb).

## Documentation map
- `docs/ARCHITECTURE.md` — modules, dependencies, data flow.
- `docs/RENDERING_PIPELINE.md` — PBR/IBL/HDR render flow and shaders.
- `docs/RESOURCE_MANAGEMENT.md` — GPU resource ownership & lifecycle.
- `docs/ROADMAP.md` — completed work + remaining improvements.
- `docs/DECISIONS.md` — architectural decisions and rationale.
- `CLAUDE.md` — agent quick-start, conventions, build, status.
