# CLAUDE.md — Agent Instructions for OpenGlCore

> Read this first. For deeper context see the `docs/` folder. Do **not** re-analyze the
> whole codebase each session — the docs below already capture the current state.

## What this project is
A **learning-oriented real-time renderer** in modern OpenGL 3.3 (Windows / Visual Studio
2022, C++20). It now implements a **metallic-roughness PBR pipeline** (Cook-Torrance) with
directional/point/spot lights, **Image-Based Lighting** (irradiance + prefilter + BRDF
LUT) from an **equirectangular HDRI** (or a procedural sky fallback), an **HDRI skybox**,
**HDR rendering with ACES tonemapping**, an FBX/OBJ/glTF model loader (Assimp), and
on-screen text (FreeType), and a **Dear ImGui editor** (Unity/Unreal-style dockable
Hierarchy / Viewport / unified component Inspector / Console). The scene is a lightweight
`Scene`/`GameObject`/`Transform` + optional components model; the Inspector edits live scene
data (transforms, materials, lights) with changes reflected immediately. The default scene
has a textured wooden chair, two spheres, and directional/point/spot lights. The code is
organized into small RAII modules; `main.cpp` is just an entry point.

## Build & run
- **Prerequisite (once)**: restore vcpkg deps (**Assimp**, **Dear ImGui** w/ glfw+opengl3
  bindings & docking, which pulls in **GLFW 3.4**) from the repo root:
  `& "C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg\vcpkg.exe" install --triplet x64-windows` → populates `vcpkg_installed/` (gitignored). FreeType is vendored.
  Note: GLFW is now linked from **vcpkg** (`glfw3dll.lib` + `glfw3.dll`) so the ImGui GLFW
  backend matches; the vendored `Libraries/lib/glfw3.lib` (3.3) is no longer linked.
- **Font (once)**: put a `.ttf` at `OpenGlCore/Fonts/arial.ttf` (gitignored; the HUD needs it).
- **HDRI environment**: an equirectangular `.exr` (via tinyexr) or `.hdr` (via stb) drives
  the skybox + IBL. Default path `./SKYBOX/ballroom_4k.exr` (set in `Application.cpp` where
  `IBL` is constructed). Missing → procedural-sky fallback, logs the expected path. EXR
  decoding uses the vendored **zlib** (now compiled into the project).
- Build/run: `./build.ps1 -Run` (or `-Rebuild`), or VS → **Debug|x64** → F5, or VS Code **Ctrl+Shift+B**.
- Assets (Shader/, Images/, Fonts/, Models/) + vcpkg DLLs are copied next to the exe on
  build, so double-click works. From a terminal the CWD must contain those folders
  (`build.ps1 -Run` handles it). SKYBOX/ is also copied for the HDRI.
- **Editor controls**: Hierarchy (left) selects objects → unified Inspector (right) shows
  Transform + Mesh Renderer + Material + Light component sections; Console (bottom). The
  3D viewport shows through the docking central node. Camera (WASD + hold-LMB look) works
  only when the cursor isn't over an ImGui panel.

## Architecture (see docs/ARCHITECTURE.md)
Entry: `main.cpp` → `Application` (owns window, scene, render loop). Modules:
- `Window` — GLFW/GLAD/context (RAII).
- `Application` — scene, HDR target, render + tonemap passes, main loop.
- `Camera`, `Mesh`, `ElementBuffer`, `VerTexArray`, `VerTexBuffer`, `Shader` — RAII GL wrappers.
- `Geometry` — procedural sphere (11-float layout).
- `Light` — `DirectionalLight`/`PointLight`/`SpotLight` + `LightSet::Upload`.
- `PbrMaterial` — metallic-roughness material (scalars + optional maps, units 3–7).
- `IBL` — builds env cubemap from an equirectangular HDRI (`.exr` via tinyexr+zlib, or
  `.hdr` via stb) rendered through `equirect.frag`, or a procedural sky fallback
  (`sky.frag`); then irradiance/prefilter/BRDF-LUT; renders the skybox (`RenderBackground`,
  intensity param). `tinyexr.cpp` compiles tinyexr against the vendored zlib.
- `Model` — Assimp loader → `Mesh` list; `Draw`/`DrawPBR`/`DrawGeometry`.
- `Text` — FreeType glyph renderer.
- `Transform`/`GameObject`/`Scene` — lightweight scene model (TRS transform + optional
  `MeshRenderer`/`LightComponent`; `std::optional` components, not an ECS).
- `Editor` — Dear ImGui panels: Hierarchy, Inspector (per-component collapsible drawers),
  Console, with a docking passthrough central viewport. `RenderSettings` holds exposure/
  skybox/IBL toggles edited from the Inspector. Camera input is ignored while ImGui has
  mouse/keyboard focus (`Editor::WantCaptureMouse/Keyboard`).

## Shaders (OpenGlCore/Shader/)
- `pbr.vert`/`pbr.frag` — main PBR (Cook-Torrance GGX/Smith/Schlick, maps, lights, IBL).
- `hdr.vert`/`hdr.frag` — fullscreen ACES tonemap + gamma.
- IBL: `cubemap.vert`, `sky.frag`, `irradiance.frag`, `prefilter.frag`, `brdf.frag`,
  `background.vert`/`background.frag`.
- Legacy (still present, used by `Model::Draw` and older demos): `default.*`, `Light.*`,
  `albedo.frag`, `text.*`.

## Conventions (preserve unless refactoring)
- **Vertex layout = 11 floats**: pos(3) color(3, unused by PBR) uv(2) normal(3). All meshes
  (sphere, model, pyramid) use it; PBR reads loc 0/2/3.
- **Texture units**: 0 irradiance, 1 prefilter, 2 BRDF LUT, 3 albedo, 4 normal, 5 metallic,
  6 roughness, 7 AO.
- Some method names keep odd casing (`LinkAttriBute`, `SetUniformValueF`) — load-bearing.
- GL wrappers are RAII: non-copyable, movable; freed in destructors. Owned via `unique_ptr`
  in `Application` so they free before the `Window` (GL context) is destroyed.

## Hard rules for agents
- Preserve existing runtime behavior; prefer small, testable changes.
- Build after changes (`./build.ps1`); no test suite — verify by running the app.
- A runtime smoke test (run a few seconds, check stderr for shader/GL errors) is cheap:
  the `Shader` class logs GLSL errors and `GLUtils.h glCheckError()` logs GL errors.

## Status
- **Done**: Phases 0–3 of the refactor roadmap (RAII, error checking, naming, Mesh,
  delta-time, shader correctness, Win32 decouple, FreeType text, Assimp models w/ node
  transforms + materials + textures), plus the **full PBR + IBL + HDR renderer** and the
  `main.cpp` → `Application` refactor. Debug|x64 builds; runtime smoke test clean
  (no shader/GL errors). Visual output not verified in this environment.

## Known issues / remaining tasks (see docs/ROADMAP.md)
- PBR texture maps are exercised by the wooden chair (BaseColor/Normal/Roughness/AO);
  the metallic-map path is still untested (no metal asset).
- IBL supports a real equirectangular `.hdr` (`Images/environment.hdr`) with a procedural
  sky fallback when the file is absent.
- No framebuffer-resize handling (fixed size from the initial window).
- `Model` has no normal/metallic/roughness map import (diffuse color/texture only).
- Legacy shaders/classes (`default.*`, `Light.*`, `Primitive`, `Texture`) remain but are
  no longer used by the main scene; candidates for removal.
