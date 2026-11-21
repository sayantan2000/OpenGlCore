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
- **Prerequisite (once)**: restore vcpkg deps (**Assimp**, **NVIDIA PhysX 5.5**, **Dear ImGui**
  w/ glfw+opengl3 bindings & docking, which pulls in **GLFW 3.4**) from the repo root
  (PhysX's first build is long):
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
- **Editor controls**: FIXED layout — Hierarchy (left), Scene Viewport (centre, the 3D shows
  through), unified Inspector (right), Console (bottom). **Left-click** an object in the
  viewport or Hierarchy to select (empty-space click deselects). Navigation (Unity-style):
  **Hold RIGHT mouse** to fly (WASD + Shift sprint, Space/Ctrl up/down, mouse look); **scroll**
  to zoom; **Alt + LEFT-drag** to orbit around the selection; **MIDDLE-drag** to pan.
  Transform gizmos (ImGuizmo): **W**
  translate, **E** rotate, **R** scale (Gizmo menu toggles world/local; **hold Ctrl to snap**).
  The selected object shows a stencil **outline**; a **grid floor** marks the XZ plane. **F** or
  Hierarchy double-click focuses the selection. **Delete** deletes, **Ctrl+D** duplicates, **Ctrl+Z/
  Ctrl+Y** undo/redo, **Ctrl+S / Ctrl+O** save/load scene (File menu; default `scene.json`).
  Right-click the Hierarchy for the Create menu (Empty / Primitive / Light); **drag** a
  Hierarchy row onto another to **reparent** (drop on empty space to unparent). **Play / Stop**
  (menu bar) enters/exits play mode: scripts run (the scene is snapshotted on Play and restored
  on Stop); add a **Script** component in the Inspector and pick a behavior (Rotator/FloatBob/
  WasdMover). Camera/tool input is ignored while over a panel or the gizmo.
- **Cameras (Unity-style)**: the editor **scene camera** (RMB-fly) is used only in Edit mode;
  **Play mode renders through the active in-scene game camera** (a `CameraComponent` on a
  GameObject — Create → Camera; the default scene has a "Main Camera"). Editor overlays
  (grid/gizmos/outline) are hidden in Play for a clean game view. Scripts can move the game
  camera like any transform.

## Project layout & conventions
- Sources live in purpose subfolders under `OpenGlCore/`: `core/` (Application, Window,
  EngineTime, GLUtils, Enums, main), `render/` (Shader, Mesh, buffers, Geometry, Camera, IBL,
  PbrMaterial, Light, Model, Text, Texture, LineRenderer, Primitive), `scene/` (Transform,
  GameObject, Scene, PrimitiveLibrary), `editor/` (Editor, EditorCommand, Picking),
  `scripting/` (Input, ScriptBehavior, ScriptRegistry, ScriptSystem), `physics/` (Physics,
  PhysicsScene, PhysicsDebug), `io/` (AssetManager, SceneIO), `thirdparty/` (glad/stb/tinyexr
  impl TUs). Each folder is on the include path, so `#include "X.h"` works across folders.
- **All engine code is in `namespace ogl`** (third-party stays global; `main()` is global and
  uses `ogl::Application`). New engine files should open `namespace ogl { … }` after includes.
- **Gameplay scripting is C++** (and will stay C++): subclass `ogl::ScriptBehavior`
  (`OnStart`/`OnUpdate`/`OnCollision`/`OnTrigger`), register it in `ScriptRegistry`, attach by
  name in the Inspector. No C#/Unity runtime, no embedded VM.
- Note: don't name an engine header after a CRT header (e.g. `Time.h` clashes with `<time.h>`
  on case-insensitive Windows once its folder is on the include path — hence `EngineTime.h`).

## Architecture (see docs/ARCHITECTURE.md)
Entry: `core/main.cpp` → `ogl::Application` (owns window, scene, render loop). Modules:
- `Window` — GLFW/GLAD/context (RAII).
- `Application` — scene, HDR target, render + tonemap passes, main loop.
- `Camera`, `Mesh`, `ElementBuffer`, `VerTexArray`, `VerTexBuffer`, `Shader` — RAII GL wrappers.
- `Geometry` — procedural sphere (11-float layout).
- `Light` — `DirectionalLight`/`PointLight`/`SpotLight`/`RectLight` (area) + `LightSet::Upload`.
- `PbrMaterial` — metallic-roughness material (scalars + optional maps, units 3–7).
- `IBL` — builds env cubemap from an equirectangular HDRI (`.exr` via tinyexr+zlib, or
  `.hdr` via stb) rendered through `equirect.frag`, or a procedural sky fallback
  (`sky.frag`); then irradiance/prefilter/BRDF-LUT; renders the skybox (`RenderBackground`,
  intensity param). `tinyexr.cpp` compiles tinyexr against the vendored zlib.
- `Model` — Assimp loader → `Mesh` list; `Draw`/`DrawPBR`/`DrawGeometry`.
- `Text` — FreeType glyph renderer.
- `Transform`/`GameObject`/`Scene` — lightweight scene model (TRS transform + optional
  `MeshRenderer`/`LightComponent`/`ScriptComponent`/`RigidBodyComponent`/`ColliderComponent`/
  `CameraComponent`; `std::optional` components, not an ECS). `Scene::ActiveCamera()` returns the
  game camera to render through in Play; `Application::UpdateActiveCamera` chooses scene vs game
  camera per frame. `GameObject` has a
  stable `id` and a `parentId`; `Scene` selects/removes/finds by id and computes
  `WorldMatrix` = parentWorld * local (cycle-guarded) so transforms nest.
- `AssetManager.{h,cpp}` — loads/caches textures & models **by path** (shared, no duplicate GPU
  uploads); materials store texture paths, mesh renderers store a `sourceId` (primitive name or
  model path) so the scene can be serialized and re-resolved.
- `SceneIO.{h,cpp}` — JSON scene save/load (nlohmann/json, vendored at
  `Libraries/source/json`). Round-trips ids, parent links, transforms, materials (+ map paths),
  all light types and script components; resolves geometry via `PrimitiveLibrary`/`AssetManager`
  on load. Also `SaveToString`/`LoadFromString` for in-memory Play-mode snapshots.
- `Input.{h,cpp}` — GLFW input abstraction (edge-detected keys/buttons, mouse pos/delta, named
  axes); polled once per frame by `Application`, read by scripts in Play mode.
- `ScriptBehavior.h` / `ScriptRegistry.{h,cpp}` / `ScriptSystem.{h,cpp}` — native C++ scripting:
  a `ScriptBehavior` base (`OnStart`/`OnUpdate` with a `ScriptContext`), a name→factory registry
  (built-ins: Rotator, FloatBob, WasdMover), and a system that owns live instances by object id
  during Play. `GameObject` stores only the behavior name + a params map (`ScriptComponent`) so
  it stays copyable/serializable. Behaviors declare tunables via `DeclareParams` (editable in
  the Inspector, serialized) and read them with `GetParam`.
- `Editor` — Dear ImGui editor: fixed Hierarchy/Inspector/Console panels + central viewport
  rect, viewport ray-picking, ImGuizmo transform gizmos, keyboard shortcuts, camera focus,
  Hierarchy Create context menu (Empty/Primitive/Light), and undo/redo. Camera/tool input is
  ignored while ImGui has focus or the gizmo is hovered/used
  (`Editor::WantCaptureMouse/Keyboard/Viewport`).
- `EditorCommand.h` — undo/redo (`CommandHistory` + `ModifyCommand`/`AddCommand`/`RemoveCommand`).
- `Picking.h` — screen→world ray + ray/AABB object picking.
- `PrimitiveLibrary.h` + `Geometry` — shared cube/sphere/plane/cylinder/cone/torus/capsule
  meshes (owned by `Application`) referenced by editor-created objects.
- `LineRenderer` — immediate-mode line overlay for light gizmos, rectangle-light outlines and
  the selection AABB (`line.vert`/`line.frag`).
- `ImGuizmo` — vendored gizmo library at `Libraries/source/ImGuizmo` (compiled into the project).
- `Physics.h`/`PhysicsScene.cpp` — **NVIDIA PhysX 5.5** (vcpkg) wrapped behind a pImpl:
  Static/Dynamic/Kinematic rigid bodies, Box/Sphere/Capsule/Plane colliders, physics materials,
  fixed-timestep sim with transform write-back, raycast/sphere-cast/overlap, contact/trigger
  events. `RigidBodyComponent`/`ColliderComponent` are plain data on `GameObject` (serializable,
  undoable). Play-mode drives it; Stop restores the JSON snapshot. See **docs/PHYSICS.md**.
- `PhysicsDebug.cpp` — collider/trigger wireframes from component data (View → Physics Debug).

## Shaders (OpenGlCore/Shader/)
- `pbr.vert`/`pbr.frag` — main PBR (Cook-Torrance GGX/Smith/Schlick, maps, lights incl. a
  representative-point **rectangle/area light**, IBL).
- `hdr.vert`/`hdr.frag` — fullscreen ACES tonemap + gamma.
- IBL: `cubemap.vert`, `sky.frag`, `irradiance.frag`, `prefilter.frag`, `brdf.frag`,
  `background.vert`/`background.frag`.
- `line.vert`/`line.frag` — editor line overlay (grid floor, light gizmos, rect-light outline).
- `outline.vert`/`outline.frag` — stencil selection silhouette (normal-extruded shell).
- `grid.vert`/`grid.frag` — procedural infinite ground grid (ray-to-plane, fwidth AA, fade, depth).
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
  transforms + materials + textures), the **full PBR + IBL + HDR renderer**, the unified
  ImGui Inspector, and the **interactive scene editor** (viewport ray-picking, ImGuizmo
  transform gizmos, W/E/R/F/Delete/Ctrl+D/Ctrl+Z/Ctrl+Y shortcuts, camera focus, fixed
  layout, Hierarchy Create menu, primitive generation, rectangle/area light, centralized
  undo/redo), and **Tier 1 toward a game engine**: an `AssetManager` (textures/models by
  path), **JSON scene serialization** (Ctrl+S/Ctrl+O), and **parent/child transform
  hierarchy** (drag-reparent; world matrices everywhere), plus **Tier 2 (in progress)**:
  **Play/Stop mode**, an **Input** system, and native **C++ scripting** (`ScriptBehavior` /
  `ScriptRegistry` / `ScriptSystem`), and **Tier 3 (in progress)**: **NVIDIA PhysX 5.5** physics
  (rigid bodies, colliders, materials, queries, collision/trigger → scripts, Play-mode sim) and a
  **procedural infinite grid** + gizmo snapping/outline. Debug|x64 builds; headless self-test
  (`OGL_SCENE_SELFTEST`) PASS — scene save/reload (incl. physics), a script tick, and a dynamic
  sphere dropping to rest on the ground. **Editor interaction and visual output were NOT verified
  in this environment (no display).**
- See `plan.md` for the full engine roadmap (Tier 1 done; Tiers 2–3 in progress; Tiers 4–6 next).

## Known issues / remaining tasks (see docs/ROADMAP.md)
- PBR texture maps are exercised by the wooden chair (BaseColor/Normal/Roughness/AO);
  the metallic-map path is still untested (no metal asset).
- IBL supports a real equirectangular `.hdr` (`Images/environment.hdr`) with a procedural
  sky fallback when the file is absent.
- `Model` has no normal/metallic/roughness map import (diffuse color/texture only).
- Legacy shaders/classes (`default.*`, `Light.*`, `Primitive`, `Texture`) remain but are
  no longer used by the main scene; candidates for removal.
- Physics: **convex/triangle mesh colliders are cooked** from retained CPU geometry
  (`Mesh`/`Model`) via PhysX standalone cooking, cached per `sourceId`, scaled via `PxMeshScale`;
  dynamic triangle meshes auto-downgrade to a convex hull (Unity-style). No character controller /
  joints / CCD yet. See docs/PHYSICS.md.
- The rectangle light is a representative-point approximation, not full LTC; gizmo rotation
  round-trips through Euler angles so very large rotations may drift slightly.
- Tier-2 leftovers: Lua scripting. Editor: multi-select, prefabs. (Event bus, gamepad input and
  a PhysX character controller are now implemented — see below.)
- **Event bus** (`core/EventBus.h`): type-safe publish/subscribe (`Subscribe<T>`/`Publish<T>`,
  `ScopedSubscription`), re-entrant-safe. Owned by `Application` (`events`), set on `ScriptSystem`
  (`SetEventBus`), reachable from scripts via `ScriptContext::events`. Physics contacts are
  re-published as `CollisionBegin/End`/`TriggerBegin/End`; also a generic `GameMessage`.
- **Gamepad**: `Input` now covers up to 4 gamepads (buttons/sticks/triggers, deadzone, connection
  detection) plus a configurable **action** layer (`BindAction`/`GetAction(Down)`); left stick is
  folded into the "Horizontal"/"Vertical" named axes.
- **Character controller**: `CharacterControllerComponent` + PhysX `PxCapsuleController` (swept
  kinematic capsule). Drive it from scripts via `PhysicsScene::MoveCharacter` /
  `IsCharacterGrounded`; built-in `CharacterController` behavior does WASD/stick + gravity + jump.
  Do NOT also add a RigidBody to a character (they are alternatives).
