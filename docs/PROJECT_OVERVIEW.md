# Project Overview

## Purpose
OpenGlCore is a **learning project** for modern real-time rendering in OpenGL 3.3 core
(Windows / VS2022 / C++20). It began as a tutorial-style demo and now implements a
**physically based rendering (PBR) pipeline** with image-based lighting, built from small
RAII modules rather than a framework.

## Current state (as of 2026-11-19)
A single executable that renders a PBR showcase scene:
- A **7×7 grid of spheres** with metallic increasing per row and roughness per column,
  lit by a **Cook-Torrance metallic-roughness** BRDF.
- **Lights**: one directional, four colored point lights (inverse-square falloff), and one
  spotlight (inner/outer cone).
- **Image-Based Lighting**: diffuse irradiance cubemap, prefiltered specular cubemap, and a
  BRDF integration LUT, all generated at startup from a **procedural HDR sky** rendered to a
  cubemap. The sky is also drawn as the skybox background.
- **HDR pipeline**: scene renders to an `RGBA16F` framebuffer, then a fullscreen pass applies
  ACES filmic tonemapping and gamma correction.
- An **Assimp-loaded model** (`Models/Cube.fbx`) drawn with the PBR shader.
- An on-screen **FPS HUD** (FreeType).
- A **fly camera** (WASD + hold-left-mouse look; Shift to sprint).

### What works (builds + runtime smoke test clean)
Window/context, PBR shading, all three light types, IBL generation + sampling, HDR tonemap,
model loading, text. The app runs without shader-compile or GL errors.

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
