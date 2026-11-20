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

## ADR-0004 — Interactive scene editor: picking, gizmos, creation, undo/redo (2026-11-20)
- **Context**: Grow the Inspector-only editor into an interactive Unity-style scene editor
  (viewport selection, transform gizmos, shortcuts, focus, fixed layout, create menus, area
  light, undo/redo).
- **Decisions**:
  - **Fixed, non-dockable layout** (replaces ADR-0003's docking passthrough): four fixed
    `ImGui` windows (`NoMove|NoResize|NoCollapse|NoBringToFrontOnFocus`) — Hierarchy left,
    Inspector right, Console bottom, and an explicit central **viewport rect** the 3D scene
    shows through. `io.IniFilename=nullptr` so positions never drift. The scene still renders
    fullscreen behind the panels (no render-to-texture), so the camera aspect never distorts;
    the central rect only defines where picking/gizmo input is accepted.
  - **ImGuizmo** (vendored at `Libraries/source/ImGuizmo`, master, ImGui-1.93 compatible) for
    translate/rotate/scale gizmos — correct interaction math, axis highlight, world/local,
    one-command-per-drag — rather than hand-rolling gizmo geometry/ray math.
  - **CPU ray/AABB picking** (`Picking.h`): screen→world ray, transformed into each object's
    local space and tested against its local AABB; nearest world hit wins. Chosen over GPU
    picking because it needs no extra render target and fits the existing fullscreen pass.
  - **Right-mouse flythrough camera** (changed from hold-LMB look): navigation (WASD + look)
    is active only while RMB is held, freeing LMB for selection/gizmo drag and W/E/R as tool
    shortcuts. This resolves the WASD-vs-tool-shortcut and click-vs-look conflicts cleanly.
  - **Centralized undo/redo** (`EditorCommand.h`, Command pattern): objects carry a stable
    `id`; `ModifyCommand` stores before/after **GameObject value snapshots** (cheap — geometry
    is referenced by non-owning pointer, so copies share handles and references stay valid),
    `Add`/`RemoveCommand` restore objects at their original index/id. One command per
    interaction via `IsItemActivated`/`IsItemDeactivatedAfterEdit` (Inspector) and
    using-edge detection (gizmo). A single history serves Inspector, gizmo and hierarchy.
  - **Primitive library** (`PrimitiveLibrary.h` + `Geometry` builders): Application owns the
    shared cube/sphere/plane/cylinder/cone/torus/capsule meshes; the editor only references
    them when creating objects, keeping all GL ownership in Application.
  - **Rectangle (area) light = representative-point approximation**, NOT full LTC: the PBR
    shader gains a `RectLight` path whose diffuse uses the closest point on the rectangle and
    whose specular uses the reflection-ray/rectangle intersection, with area-scaled,
    one-sided, inverse-square falloff. This is physically motivated and honest (documented as
    an approximation); full LTC was judged too heavy (needs the Heitz LUT tables) for the
    learning scope. A wireframe overlay (`LineRenderer`) visualizes it.
- **Status**: Accepted and implemented. Debug|x64 builds clean; runtime smoke test clean (no
  shader/GL errors, HDRI loaded, 7 s no crash). Editor **interaction and visuals were not
  verified** (no display in the dev environment).
- **Known limitations**: gizmo rotation round-trips through the Transform's Euler angles
  (glm order), so very large rotations can show minor drift vs ImGuizmo's internal order;
  no gizmo snapping UI; selection highlight is an AABB, not a mesh outline; no scene
  serialization yet (Ctrl+S logs a notice).

## ADR-0005 — Tier 1 engine foundations: assets, serialization, hierarchy (2026-11-20)
- **Context**: First step from "renderer + editor" toward a game engine — make scenes
  persistent and composable (see `plan.md`).
- **Decisions**:
  - **AssetManager** caches textures/models **by path** (shared GPU uploads, negative-cache on
    failure). Materials now carry texture *paths* beside their GL handles, and mesh renderers
    carry a `sourceId` (primitive name or model path). This is the prerequisite that lets the
    scene be serialized by reference instead of by opaque handle. The default chair + spheres
    were rerouted through it so the whole scene is serializable.
  - **JSON serialization** via **nlohmann/json** (single header, vendored at
    `Libraries/source/json`) rather than a hand-rolled format — ubiquitous, header-only, no
    build cost. `SceneIO` round-trips ids, parent links, transforms, material scalars + map
    paths, and every light type; geometry/textures are re-resolved on load. A guarded
    `OGL_SCENE_SELFTEST` path saves→reloads the built scene headlessly (verified: 8→8 objects,
    chair geometry + maps re-resolved), since there's no display to drive Ctrl+S.
  - **Parent/child hierarchy** by `parentId` + `Scene::WorldMatrix` (not a stored world matrix
    or a pointer tree) — simplest, keeps the flat object list and stable-id model intact, and
    composes on demand with a depth guard against cycles. The gizmo manipulates the world
    matrix and converts back to a parent-relative local transform so nested objects stay
    correct. Delete promotes children to the deleted node's parent.
- **Status**: Accepted and implemented. Debug|x64 builds clean; normal run smoke-test clean;
  serialization round-trip self-test **PASS**. Editor interaction/visuals not verified (no display).
- **Known limitations**: reparenting keeps the local transform (so the object may visually
  jump — no world-preserving reparent yet); undo of a delete doesn't restore former child
  links (children were promoted); no file dialog (fixed `scene.json` path); textures are not
  yet sRGB-flagged at upload (shader still does the decode).

## ADR-0006 — Tier 2 engine runtime: play mode, input, native scripting (2026-11-20)
- **Context**: Make the editor *run* a scene, not just edit one (see `plan.md` Tier 2).
- **Decisions**:
  - **Play/Stop via a JSON snapshot** rather than a deep-copy clone or a parallel runtime
    scene: on Play the scene is serialized to a string (`SceneIO::SaveToString`) and on Stop
    restored (`LoadFromString`), so play-time mutations are discarded exactly. Reuses the Tier 1
    serializer — no new state model. Undo history is cleared on stop (objects were rebuilt).
  - **Native C++ scripting first**, not Lua/sol2: a `ScriptBehavior` base with `OnStart`/
    `OnUpdate(ScriptContext)`, a `ScriptRegistry` (name→factory), and a `ScriptSystem` that owns
    live instances **keyed by object id**. This keeps `GameObject` copyable/serializable (it
    stores only the behavior name in `ScriptComponent`), which is required by the undo snapshots
    and the serializer. Lua is deferred until a scripting need actually justifies the dependency
    and build cost.
  - **Instances live only during Play** (created in `Begin`, destroyed in `End`) so edit-mode
    stays pure data and behaviors can hold per-run state (e.g. FloatBob's base height).
  - **Input abstraction** (`Input`) wraps GLFW with edge detection + named axes, polled once per
    frame, so scripts don't poll GLFW directly and the camera keeps its own direct polling.
- **Status**: Accepted and implemented. Debug|x64 builds; normal run clean; headless self-test
  (`OGL_SCENE_SELFTEST`) exercises save→reload **and** runs the Rotator behavior 10 ticks
  (dy=9.6° = 60°/s·0.16s) — **PASS**. Play/Stop *UI* not visually verified (no display).
- **Known limitations**: no Lua; script parameters aren't yet editable in the Inspector (only
  the behavior name); no gamepad; no event/messaging system yet; objects spawned during Play
  get no script instance until the next Play.

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
