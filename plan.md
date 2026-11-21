# OpenGlCore — Roadmap to a Game Engine

Current state: OpenGL 3.3 **PBR + IBL + HDR** renderer with an interactive **Dear ImGui +
ImGuizmo** scene editor (viewport picking, transform gizmos, shortcuts, camera focus, fixed
layout, Create menu, primitive generation, rectangle/area light, centralized undo/redo).
Scene model = lightweight `Scene` / `GameObject` / `Transform` + optional components (no ECS).

The tiers below are ordered so each one unblocks the next.

---

## Tier 1 — Close the editor loop  ✅ DONE (2026-11-20)
Make what already exists persistent and composable. Highest return, low risk.
- [x] **Asset manager** (`AssetManager.{h,cpp}`) — loads/caches textures & models **by path**
      (no duplicate GL uploads); materials carry texture **paths**, meshes carry a source id
      (primitive name / model path), resolved through the manager. The chair + spheres now go
      through it.
- [x] **Scene serialization** (`SceneIO.{h,cpp}`, nlohmann/json vendored) — saves/loads the
      whole `Scene` (objects, ids, parent links, transforms, material scalars + texture paths,
      all light types) and re-resolves geometry/textures on load. **Ctrl+S / Ctrl+O** +
      File menu (New/Save/Load). Round-trip verified headlessly (`OGL_SCENE_SELFTEST`).
- [x] **Parent/child hierarchy** — `GameObject.parentId`; `Scene::WorldMatrix` = parentWorld *
      local (cycle-guarded); Hierarchy is a tree with **drag-to-reparent** (+ Unparent);
      picking, gizmo (world→local on drag), focus and rendering all use world matrices.

## Tier 2 — Engine runtime (the "game" part)  ⏳ IN PROGRESS
- [x] **Play / Stop mode** (`Editor` + `ScriptSystem`): snapshot the scene to JSON on Play,
      run scripts each frame, restore on Stop. Play/Stop button in the menu bar.
- [x] **Scripting / behaviors** — native C++ `ScriptBehavior` (`OnStart`/`OnUpdate`) +
      `ScriptRegistry` (name→factory, serializable `ScriptComponent`) + `ScriptSystem` (owns
      live instances by object id during Play). Example behaviors: Rotator, FloatBob, WasdMover.
      *(Lua/sol2 scripting deferred — native interface first.)*
- [x] **Input system** (`Input.{h,cpp}`) — edge-detected keys/buttons, mouse pos/delta, named
      axes (Horizontal/Vertical), **gamepad** (up to 4 pads via GLFW: buttons/sticks/triggers,
      deadzone, connection detection, sticks folded into the named axes), and a **configurable
      action layer** (name→keys/mouse/pad buttons; defaults Jump/Fire/Sprint/Cancel/Submit).
- [x] **Event/messaging system** (`core/EventBus.h`) — type-safe publish/subscribe keyed by
      `std::type_index`; re-entrant-safe dispatch + deferred unsubscribe (no dangling callbacks);
      `ScopedSubscription` RAII; built-in Collision/Trigger/PlayMode events + generic `GameMessage`.
      Exposed to scripts via `ScriptContext::events`; physics contacts are re-published on the bus.
- [ ] *(Defer)* **Real ECS (EnTT)** — only once component variety/entity counts justify it.
- [x] **Script parameters in the Inspector** — behaviors declare params (`DeclareParams`),
      per-object values live in `ScriptComponent.params` (serialized), editable as DragFloats;
      behaviors read them via `GetParam`.
- [ ] *(Next)* Lua scripting.

## Tier 3 — Physics & spatial systems  ⏳ IN PROGRESS
- [x] **Physics — NVIDIA PhysX 5.5** (via vcpkg): `PhysicsScene` wraps PhysX behind a pImpl.
      Static/Dynamic/Kinematic rigid bodies; Box/Sphere/Capsule/Plane colliders; physics
      materials (friction/restitution); mass/gravity/damping/constraints (lock flags);
      fixed-timestep simulation with transform write-back; Play-mode start/stop with JSON
      snapshot restore. Raycast / sphere cast / overlap queries. Collision + trigger callbacks.
- [x] **Collision / trigger callbacks** → scripting: `ScriptBehavior::OnCollision/OnTrigger`,
      dispatched by `ScriptSystem` from `PhysicsScene::Events()`. Scripts also get a
      `PhysicsScene*` in their context (raycast, set velocity, add force/impulse).
- [x] **Physics debug visualization** — collider/trigger wireframes (`PhysicsDebug`, component
      -based, works in edit + play), toggle in the View menu.
- [x] **Mesh colliders** (Convex / Triangle) — cooked from retained CPU geometry
      (`Mesh`/`Model`) via PhysX standalone cooking, cached per `sourceId`, scaled with
      `PxMeshScale`; dynamic triangle meshes auto-downgrade to a convex hull. Validated by the
      headless self-test (ConvexMesh cook + drop settles at y≈-0.45).
- [x] **Character controller** — PhysX `PxCapsuleController` (swept kinematic capsule, not a
      force-driven rigid body): `CharacterControllerComponent` (radius/height/slope/step/skin +
      move/run/jump/gravity tuning), `PhysicsScene::MoveCharacter/IsCharacterGrounded`, a built-in
      `CharacterController` script (WASD/stick + gravity + jump + ground detection), Inspector
      section, SceneIO serialization, and capsule debug viz. Validated headlessly (capsule falls,
      grounds, settles).

## Tier 4 — Rendering depth
- [ ] **Shadow mapping** — directional (cascaded) first, then spot/point. Biggest realism jump.
- [ ] **Frustum culling + spatial structure** (BVH/octree) once scenes are large.
- [ ] **Post-processing stack** — bloom, SSAO, FXAA/TAA on top of the existing HDR pass.
- [ ] **Transparency / alpha-blended pass** (everything is opaque today).
- [ ] **Full LTC area lights** (current rectangle light is a representative-point approximation).
- [ ] **Render-to-texture viewport** so the editor viewport is a real docked panel.
- [ ] **Instanced rendering** for repeated meshes.

## Tier 5 — Content pipeline & polish
- [x] **Asset database + GUID references** (`io/AssetDatabase`, `io/Guid.h`) — scans Assets/,
      `.meta` sidecars with stable GUIDs, GUID<->path maps, Refresh/RegisterNewAsset, move/rename
      reference preservation. See docs/ASSET_PIPELINE.md.
- [x] **Material assets** (`render/MaterialAsset`) — persistent `.mat` (GUID, scalars, texture refs
      by GUID, tiling/offset), Resolve()→PbrMaterial. Material Inspector + Project browser + Create/
      Assign in the editor. See docs/MATERIAL_SYSTEM.md, docs/EDITOR.md.
- [x] **Project system + Engine Hub** — standard project structure, ProjectSettings, Main.scene
      seeding by template; standalone `EngineHub.exe` (create/open/recent + launches the editor via
      `--project`). See docs/PROJECT_SYSTEM.md.
- [ ] **Model PBR-map import** in `Model` (normal/metallic/roughness/AO, not just diffuse).
- [ ] **Prefabs** — reusable GameObject templates instantiated into scenes.
- [x] **Editor camera navigation** — scroll-zoom, Alt+LMB orbit around selection, middle-mouse
      pan (Unity-style), complementing RMB flythrough.
- [x] **Grid floor** (XZ grid + coloured X/Z axes) in the viewport.
- [x] **Gizmo snapping** — hold Ctrl to snap (0.5u / 15° / 0.1).
- [x] **Mesh-outline selection** — stencil silhouette around the selected object (replaces the
      AABB box).
- [x] **Procedural infinite grid** — camera-independent ground grid in a fragment shader
      (`grid.vert/frag`): ray-to-plane per pixel, fwidth AA, major/minor lines, coloured X/Z
      axes, distance fade, correct depth; grid spacing drives gizmo snapping. Replaces the old
      fixed line-mesh grid.
- [ ] **Editor UX (remaining)** — multi-select, scene-tree multi-drag, prefabs.
- [ ] **Audio** (miniaudio / OpenAL) — 2D then 3D positional.
- [ ] **In-game UI / text** rendering (the FreeType HUD generalized to a UI layer).
- [ ] **Hot-reload** for shaders, then assets.

## Tier 6 — Engineering & shipping (ongoing, not a gate)
- [ ] **Logging** system (levels, categories) replacing ad-hoc `std::cerr`/console pushes.
- [ ] **Math/util library** consolidation (AABB, ray, frustum, transform helpers in one place).
- [ ] **Unit tests** for pure logic (serialization round-trip, math, command undo/redo).
- [ ] **CMake build** (alongside/instead of the `.vcxproj`) for portability & CI.
- [ ] **Cross-platform** cleanup (already mostly GLFW-based; audit remaining Win-only bits).
- [ ] **Asset packaging / build step** for distributing a game.
- [ ] *(Optional, late)* **Networking**.

---

## Sequencing recommendation
Finish **Tier 1 fully** before anything else — without save/load and a scene graph, every
later system is painful to test. Then pick the fork:
- Want it to **look** like an engine → **shadow mapping** (Tier 4).
- Want it to **behave** like one → **play-mode + scripting** (Tier 2).

> Validation note: there is no display in the dev environment, so features are validated by
> build + runtime smoke test (run the exe, check stderr for shader/GL errors). Visual and
> interaction behavior must be confirmed by running `./build.ps1 -Run`.
