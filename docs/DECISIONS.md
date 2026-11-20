# Architectural Decisions

> Records decisions and their rationale. Append new entries; don't rewrite history.

## ADR-0001 — Documentation-first baseline (2026-11-19)
- **Context**: Codebase had no docs (`CLAUDE.md`/`README.md` empty). A full architectural
  assessment was requested before any refactoring.
- **Decision**: Capture the *current* implementation in `docs/` + `CLAUDE.md`, and record
  the proposed refactor as an un-executed, prioritized `ROADMAP.md`. No source changed.
- **Rationale**: Lets future sessions (human or AI) understand the project without
  re-analyzing everything, and separates "what is" from "what should be".
- **Status**: Accepted. Roadmap items remain **unapproved** pending maintainer review.

## ADR-0002 — PBR renderer architecture (2026-11-19)
- **Context**: Add a complete metallic-roughness PBR pipeline with IBL and HDR, and
  de-clutter `main.cpp`.
- **Decisions**:
  - **`main.cpp` → `Application` + `Window`**: entry point is minimal; `Application` is a
    facade composing the subsystems; `Window` is the RAII GL-context owner. GL resources are
    held in `unique_ptr`s so they free before the context.
  - **Metallic-roughness workflow** (not specular-glossiness) — industry default, simpler.
  - **IBL from a procedural sky** initially, then (update 2026-11-19) an **equirectangular
    `.hdr` loader** (`equirect.frag` + stb `loadf`) as the primary source with the procedural
    sky as fallback. The HDRI env cubemap is reused for the skybox and all IBL products (no
    duplicate processing). Skybox intensity is independent of HDR exposure.
  - **Normal mapping via screen-space derivatives** — avoids adding a tangent vertex
    attribute, keeping the single 11-float layout used everywhere.
  - **Lights as plain structs + `Upload`**, materials as `PbrMaterial` + `Bind` — no deep
    class hierarchy; matches the "avoid overengineering" principle.
  - **HDR `RGBA16F` + ACES tonemap** in a fullscreen post pass; lighting in linear space,
    sRGB decode on albedo textures, gamma at the end.
  - **No new third-party dependency** was introduced for PBR/IBL.
- **Status**: Accepted and implemented. Builds Debug|x64; runtime smoke test clean (no
  shader/GL errors). Visual correctness not verified in the dev environment.

## ADR-0003 — Editor + scene model (Dear ImGui) (2026-11-20)
- **Context**: Add a Unity/Unreal-style unified Inspector that edits real scene data.
- **Decisions**:
  - **Lightweight scene model**, not an ECS: `GameObject` with a `Transform` and
    `std::optional<MeshRenderer>` / `std::optional<LightComponent>`. Matches the project's
    "avoid overengineering" principle while giving the Inspector real components to edit.
  - **Dear ImGui via vcpkg** (docking + glfw/opengl3 bindings). The Inspector is one panel
    with reusable per-component drawer functions (Transform/MeshRenderer/Material/Light),
    not separate editor windows.
  - **Docking passthrough central node**: the existing 3D render stays as the fullscreen
    pass to the default framebuffer; ImGui panels dock around a transparent center, so no
    render-to-texture viewport was needed (simpler, keeps the render path intact).
  - **GLFW switched to vcpkg's 3.4** (dynamic) because the ImGui GLFW backend requires
    `glfwGetPlatform` (3.4); the vendored 3.3 static lib is no longer linked.
  - **Camera input gated** by `ImGuiIO::WantCaptureMouse/Keyboard` so UI interaction never
    moves the camera. Editor UI is kept out of the render path (separate `Editor` class).
  - Lights are first-class `GameObject`s; the renderer rebuilds the `LightSet` from them
    each frame (single source of truth = the scene).
- **Status**: Accepted and implemented. Debug|x64 builds; runtime smoke test clean. Editor
  interaction not visually verified in the dev environment.

## Pre-existing decisions (reconstructed from the code — not formally recorded before)
These describe why the project looks the way it does; confirm/revise with the maintainer.

- **Thin wrappers over raw GL, no engine framework.** Intentional for learning: the point
  is to see the GL calls, not hide them. → Keep wrappers thin; avoid over-abstraction.
- **GLFW + GLAD + GLM + stb**, vendored under `Libraries/source`. Standard, minimal modern
  OpenGL stack. Accepted.
- **OpenGL 3.3 core profile.** Broad compatibility; sufficient for the learning goals.
- **Immediate, hard-coded scene in `main.cpp`.** Acceptable at current scale; see Roadmap
  2.2 if object count grows.
- **Windows-only** (`Windows.h` for monitor metrics). Convenience, not a hard requirement;
  Roadmap 3.3 proposes GLFW-native alternatives if portability becomes a goal.

## Open questions for the maintainer
1. Is the **sphere** meant to be a plain light-source marker (position-only) or a fully
   lit/textured object? (Affects Roadmap 0.2 scope.)
2. Are **freetype/zlib** intended future features (text rendering) or leftovers to remove?
3. Is **model loading** (FBX/other) a real goal, or is `Cube.fbx` experimental?
4. Should the project stay Windows-only or move toward portability?
