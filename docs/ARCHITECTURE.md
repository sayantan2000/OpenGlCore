# Architecture

> Describes the **actual current implementation**. Proposed/remaining work is in
> `docs/ROADMAP.md`.

## High-level shape
`main.cpp` is a minimal entry point that constructs an `Application` and calls `Run()`.
`Application` owns the window, render targets, scene and main loop, and wires together the
PBR + IBL + HDR pipeline. Everything else is a focused module: RAII GL-resource wrappers,
lighting, materials, geometry, IBL, model loading, and text. GL-owning members live in
`std::unique_ptr`s inside `Application` so they are destroyed **before** the `Window`
(the GL context).

```
main.cpp
  └─ Application ── owns ──> Window (GLFW/GLAD/context)
        ├─ Camera                 view/projection + input
        ├─ Shader (pbr, hdr)      program wrappers
        ├─ IBL                    irradiance + prefilter + BRDF LUT + skybox
        ├─ Mesh (sphere)          VAO+VBO+EBO
        ├─ Model (Cube.fbx)       Assimp -> Mesh list (+ materials/textures)
        ├─ Text (HUD)             FreeType glyph renderer
        ├─ LightSet               directional + point[] + spot[]
        └─ HDR framebuffer        RGBA16F color + depth RBO
```

## Module reference

| File | Role | Notes |
|------|------|------|
| `main.cpp` | Entry point | try/catch around `Application`. |
| `Application.{h,cpp}` | Scene, HDR target, render + tonemap passes, loop | The pipeline wiring. GL members via `unique_ptr`. |
| `Window.{h,cpp}` | GLFW init, window, context, GLAD (RAII) | `FULLSCREEN` constant in `Application.cpp`. |
| `Camera.{h,cpp}` | View/projection + WASD/mouse, delta-time scaled | Adds `GetViewMatrix`/`GetProjectionMatrix` for the skybox pass. |
| `Shader.{h,cpp}` (`shaderClass`) | Program build + uniforms (RAII) | Logs GLSL compile/link errors. |
| `Mesh.{h,cpp}` | VAO+VBO+EBO for one drawable; `Draw()` | 11-float layout via `VertexAttribute`. |
| `VerTexArray/VerTexBuffer/ElementBuffer` | RAII VAO/VBO/EBO | non-copyable, movable. |
| `Geometry.{h,cpp}` | Procedural UV sphere (11-float) | Used for the material grid. |
| `Light.{h,cpp}` | Light structs + `LightSet::Upload(shader)` | Directional/Point/Spot/**Rect** (area); `MAX_POINT=8`, `MAX_SPOT=4`, `MAX_RECT=4`. |
| `PbrMaterial.{h,cpp}` | Metallic-roughness material | Scalars + optional maps (units 3–7). |
| `IBL.{h,cpp}` | Builds irradiance/prefilter/BRDF-LUT from `sky.frag`; skybox | Owns cubemaps/LUT/cube VAO; frees in dtor. |
| `Model.{h,cpp}` | Assimp loader → `Mesh` list | Node transforms baked; material color + diffuse texture; `Draw` (albedo) / `DrawPBR`. |
| `Text.{h,cpp}` | FreeType glyph renderer | 2D overlay. |
| `Transform.h` | TRS transform (Euler degrees) | `Matrix()` = T*R*S; `Reset()`. |
| `GameObject.h` | Scene object: name, active, `Transform`, optional components | `std::optional<MeshRenderer>` + `std::optional<LightComponent>` (no ECS). |
| `Scene.h` | Flat GameObject list + selection by stable id | Add/InsertAt/RemoveById/FindById; stable `id` per object; selection survives create/delete/undo. |
| `Editor.{h,cpp}` | Dear ImGui editor: fixed layout, picking, gizmos, shortcuts, focus, create menus, undo/redo | Hierarchy/Inspector/Console fixed panels + central viewport rect; per-component Inspector drawers with per-interaction undo; gates camera via WantCaptureMouse/Keyboard/Viewport. |
| `EditorCommand.h` | Undo/redo command pattern + `CommandHistory` | `ModifyCommand` (before/after GameObject snapshot), `Add`/`RemoveCommand`; bounded stacks. |
| `Picking.h` | Screen→world ray + ray/AABB (local-space) | `ScreenPointToRay`, `RayObjectAABB`; nearest hit for click selection. |
| `PrimitiveLibrary.h` | Registry of shared primitive meshes for the Create menu | `PrimitiveDef{mesh, localMin/Max, name}`; meshes owned by Application. |
| `LineRenderer.{h,cpp}` | Immediate-mode line overlay (RAII, own shader) | Light gizmos, rectangle-light outline, selection AABB (`line.vert/frag`). |
| `ImGuizmo.{h,cpp}` (vendored) | Translate/rotate/scale gizmos | `Libraries/source/ImGuizmo`; drives the viewport transform handles. |
| `Texture.{h,cpp}`, `Primitive.{h,cpp}` | Legacy (stb texture, pyramid/sphere data) | Not used by the PBR scene; kept for now. |
| `glad.c`, `stb.cpp` | Third-party single-file impls | Leave as-is. |

## Dependencies
- **GLFW** (window/input), **GLAD** (GL loader), **GLM** (math, header-only), **stb_image**
  (texture decode, incl. embedded + HDR-capable), all vendored under `Libraries/source`.
- **FreeType** — text (vendored prebuilt libs + headers in `Libraries/`).
- **Assimp** — model import (via vcpkg manifest `vcpkg.json`; restored to `vcpkg_installed/`).
- **tinyexr** (vendored header, `tinyexr.cpp`) + **zlib** (vendored source, now compiled) —
  OpenEXR (`.exr`) HDR environment loading.
- **Dear ImGui** (via vcpkg, docking + glfw/opengl3 bindings) — the editor UI. Pulls in
  **GLFW 3.4** (vcpkg `glfw3dll.lib`/`glfw3.dll`), which is now the linked GLFW (the
  vendored 3.3 static lib is unused) so the ImGui GLFW backend matches.
- **opengl32.lib** — system GL. No Win32 dependency in source (GLFW handles the monitor).
- IBL uses a **procedural sky**, so no `.hdr`/HDR-image dependency.

## Editor interaction (selection, gizmos, focus, shortcuts)
- **Layout**: fixed, non-movable panels sized from the display each frame — Hierarchy
  (left 300px), Inspector (right 340px), Console (bottom 190px), and a computed central
  **viewport rect**. The 3D scene renders fullscreen behind the panels; the viewport rect
  only bounds where picking and the gizmo accept input (camera aspect is never distorted).
  On framebuffer resize, `Application::ResizeIfNeeded` recreates the HDR target and updates
  the camera dimensions.
- **Picking** (`Picking.h`): on a left-click that isn't a drag (<5px) and isn't over a panel
  or the gizmo, build a ray from the mouse via `inverse(proj*view)` (near/far unprojection),
  transform it into each active mesh object's local space with `inverse(model)`, and slab-test
  it against the object's local AABB. The nearest world-space hit is selected; empty space
  deselects. Model objects use the imported Assimp bounds; primitives use their known bounds.
- **Gizmos** (ImGuizmo): drawn on the background draw list clipped to the viewport rect using
  the frame's `view`/`proj`. The object's TRS matrix is manipulated, then decomposed back to
  position / Euler-degrees / scale into the `Transform`. A drag is committed as **one** undo
  command by snapshotting the pre-drag state on the last non-using frame and pushing a
  `ModifyCommand` on the using→idle edge. `W/E/R` switch translate/rotate/scale; the Gizmo
  menu toggles world/local.
- **Camera focus** (`F`, or double-click in the Hierarchy): compute the selection's
  world-space AABB (8 transformed corners), take its center and bounding radius, and place the
  camera along its current view direction at `distance = radius / tan(fovY/2) * 1.4` (clamped),
  preserving orientation so small and large objects are framed consistently.
- **Shortcuts** (gated off while typing in a field or while RMB-flythrough is active):
  `W/E/R` tools, `F` focus, `Delete` delete, `Ctrl+D` duplicate, `Ctrl+Z/Ctrl+Y` undo/redo,
  `Ctrl+S` (logs "not implemented" — no serialization yet).
- **Undo/redo** (`EditorCommand.h`): one shared `CommandHistory`. Inspector edits use
  `IsItemActivated`/`IsItemDeactivatedAfterEdit` to group a slider/drag/color interaction into
  a single `ModifyCommand`; discrete controls (checkbox/combo/reset) and create/duplicate/
  delete push their commands directly. Commands resolve objects by stable `id`, so references
  (shared meshes/materials/textures) stay valid across undo of deletion.
- **Area light**: the rectangle light is shaded with a representative-point approximation in
  `pbr.frag` (`shadeRect`) — closest-point diffuse + reflection-ray specular, area-scaled,
  one-sided, inverse-square. `LineRenderer` draws its outline + normal. Not full LTC; see
  ADR-0004.

## Design patterns (deliberately lightweight)
- **RAII** for every GL resource (rule-of-five wrappers; `unique_ptr` ownership in `Application`).
- **Facade**: `Application` is the one place the subsystems are composed.
- **Plain data + upload**: `LightSet`/`PbrMaterial` are POD-ish structs with an upload/bind
  method, not deep class hierarchies — avoids overengineering for a learning project.
