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
      axes (Horizontal/Vertical); polled once per frame, read by scripts. *(Gamepad TODO.)*
- [ ] **Event/messaging system** — decoupled communication (collision events, UI events, etc.).
- [ ] *(Defer)* **Real ECS (EnTT)** — only once component variety/entity counts justify it.
- [ ] *(Next)* Lua scripting, script parameters exposed in the Inspector, gamepad input.

## Tier 3 — Physics & spatial systems
- [ ] **Physics** (Jolt or Bullet) — rigid bodies, colliders, raycasts (reuse the editor ray math).
- [ ] **Collision / trigger callbacks** feeding the scripting/event layer.
- [ ] **Character controller** (kinematic) for player movement.

## Tier 4 — Rendering depth
- [ ] **Shadow mapping** — directional (cascaded) first, then spot/point. Biggest realism jump.
- [ ] **Frustum culling + spatial structure** (BVH/octree) once scenes are large.
- [ ] **Post-processing stack** — bloom, SSAO, FXAA/TAA on top of the existing HDR pass.
- [ ] **Transparency / alpha-blended pass** (everything is opaque today).
- [ ] **Full LTC area lights** (current rectangle light is a representative-point approximation).
- [ ] **Render-to-texture viewport** so the editor viewport is a real docked panel.
- [ ] **Instanced rendering** for repeated meshes.

## Tier 5 — Content pipeline & polish
- [ ] **Model PBR-map import** in `Model` (normal/metallic/roughness/AO, not just diffuse).
- [ ] **Prefabs** — reusable GameObject templates instantiated into scenes.
- [ ] **Editor UX** — gizmo snapping, multi-select, stencil mesh-outline selection, orbit/pan camera.
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
