# Refactoring Roadmap

> Phases 0–3 (refactor foundations) and the **PBR renderer** (Phase 4) are **DONE**.
> Remaining work is listed under "Phase 5 — Remaining / future" at the bottom.

---

## Phase 4 — PBR + IBL + HDR renderer — ✅ DONE (2026-11-19, Debug|x64 builds; runtime smoke test clean)
- **main.cpp refactor**: reduced to an entry point; all setup/loop/teardown moved into
  `Application`, with `Window` owning GLFW/GLAD/context. GL objects owned via `unique_ptr`
  so they free before the context.
- **PBR** (`pbr.vert`/`pbr.frag`): Cook-Torrance metallic-roughness — GGX NDF, Smith
  geometry, Schlick Fresnel, `F0=mix(0.04,albedo,metallic)`, energy-conserving `kD`.
  Supports albedo/normal/metallic/roughness/AO maps (units 3–7; normal mapping via
  screen-space derivatives). Scene demonstrates it with a 7×7 metallic/roughness grid.
- **Lights** (`Light.{h,cpp}`): directional + point (inverse-square) + spot (inner/outer
  cone), multiple simultaneous (`MAX_POINT_LIGHTS=8`, `MAX_SPOT_LIGHTS=4`).
- **IBL** (`IBL.{h,cpp}` + `sky/irradiance/prefilter/brdf/cubemap/background` shaders):
  procedural HDR sky → env cubemap → irradiance + prefiltered specular + BRDF LUT, sampled
  in `pbr.frag`; skybox background.
- **HDR** (`hdr.vert`/`hdr.frag`): scene → `RGBA16F` FBO → ACES tonemap + gamma.
- **Materials** (`PbrMaterial.{h,cpp}`): scalar + optional maps.
- See `docs/RENDERING_PIPELINE.md` for the full flow.

## Phase 6 — Editor: unified component Inspector (Dear ImGui) — ✅ DONE (2026-11-20)
- Added a lightweight scene model: `Transform`, `GameObject` (optional `MeshRenderer` /
  `LightComponent` components), `Scene` (list + selection). `Application` builds the scene
  (chair + 2 spheres + 4 lights), renders by iterating it, and gathers the `LightSet` from
  light objects each frame.
- `Editor` (Dear ImGui, docking) draws a **Scene Hierarchy** (left), a **unified Inspector**
  (right) with collapsible per-component sections (Transform, Mesh Renderer, Material w/
  texture previews, Light) via reusable drawer functions, and a **Console** (bottom). The 3D
  viewport shows through the docking passthrough central node. Inspector edits mutate live
  scene data (material/transform/light) with immediate viewport effect; Render Settings
  (exposure, skybox intensity, IBL/skybox toggles) are editable too.
- Selection stays valid on delete; "nothing selected" is handled. Camera input is ignored
  while ImGui has mouse/keyboard focus.
- Build: links vcpkg ImGui (static) + vcpkg GLFW (dynamic). Debug|x64 builds; runtime smoke
  test clean. Editor interaction not visually verified in this environment.

## Phase 7 — Interactive scene editor (selection, gizmos, creation, undo/redo) — ✅ DONE (2026-11-20)
- **Fixed editor layout** (replaces docking): non-movable Hierarchy / Inspector / Console +
  a central viewport rect the 3D shows through; `Application::ResizeIfNeeded` rebuilds the
  HDR target and updates the camera on framebuffer resize.
- **Viewport object selection** (`Picking.h`): left-click ray-picks the nearest object via
  ray/local-AABB; empty-space click deselects; selection syncs Hierarchy + Inspector.
- **Transform gizmos** (vendored **ImGuizmo**): W/E/R translate/rotate/scale, world/local,
  axis highlight; each drag is one undo command.
- **Camera** switched to **right-mouse flythrough** so LMB is free for selection/gizmo and
  W/E/R are tool shortcuts. **F** / Hierarchy double-click **focuses** the selection (frames
  its world AABB, preserving view direction).
- **Shortcuts**: W/E/R, F, Delete, Ctrl+D (duplicate), Ctrl+Z/Ctrl+Y (undo/redo), Ctrl+S
  (logs "serialization not implemented"); gated off while typing / flythrough.
- **Hierarchy Create context menu**: Empty / Primitive (cube, sphere, plane, cylinder,
  capsule, cone, torus — new `Geometry` builders + `PrimitiveLibrary`) / Light (directional,
  point, spot, **rectangle**). New objects auto-select and show in the Inspector.
- **Rectangle/area light**: representative-point approximation in `pbr.frag` (NOT full LTC;
  documented), with a `LineRenderer` wireframe overlay (+ light gizmos, selection AABB).
- **Centralized undo/redo** (`EditorCommand.h`): stable object ids; `ModifyCommand`
  (before/after snapshots) for Inspector + gizmo edits, `Add`/`RemoveCommand` for create/
  duplicate/delete; single history; grouped per interaction.
- Build: compiles ImGuizmo + new modules. Debug|x64 builds; runtime smoke test clean.
  **Editor interaction not visually verified (no display).**
- **Remaining**: scene serialization (Ctrl+S), full LTC area light, gizmo snapping, mesh
  outline selection highlight, undo-history panel polish.

## Phase 5 — Remaining / future (Low–Medium)
- ✅ **PBR texture-mapped mesh**: the wooden chair (`Models/wooden_chair_ukipagtbw_high/`)
  renders with BaseColor/Normal/Roughness/AO maps (metallic=0) via `Model::DrawGeometry` +
  an external `PbrMaterial`. (Metallic-map path still unexercised — no metal asset.)
- ✅ **IBL from a loaded equirectangular `.hdr`** (`equirect.frag` + stb `loadf`): the IBL
  env source is the HDRI at `Images/environment.hdr` when present (procedural sky fallback
  otherwise); the same env cubemap drives the skybox + irradiance/prefilter/BRDF. Skybox
  intensity is independently controllable (`u_bgIntensity`). Supply the `.hdr` to use it.
- ✅ **Framebuffer resize** handling (`Application::ResizeIfNeeded` recreates the HDR target
  and updates the camera on resize; Text projection is still fixed-size).
- **Model PBR map import** in `Model` (normal/metallic/roughness/AO, not just diffuse).
- Remove now-unused legacy shaders/classes (`default.*`, `Light.*`, `Primitive`, `Texture`)
  once confirmed obsolete.
- Optional: shadow mapping, MSAA/bloom, per-light debug gizmos.

---

## Earlier phases (0–3) — ✅ DONE — history below

Guiding principle: this is a learning project. Fix correctness first, add a *small* amount
of safety/structure, and avoid heavy engine abstractions that obscure the GL learning.

---

## Phase 0 — Correctness bugs (Critical) — ✅ DONE (2026-11-19, Debug|x64 builds)
Fixed in: `Primitive.cpp` (0.1, 0.2), `Texture.cpp` (0.3), `main.cpp` (0.4).

### 0.1 `Primitive` switch fall-through
- **Problem**: `Primitive.cpp` `case sphere:` has no `break`, so constructing a sphere
  also runs `GeneratePyramid()`, appending pyramid data to the sphere's vectors.
- **Why**: The "sun" sphere's buffers are corrupted; wrong geometry uploaded.
- **Proposal**: Add `break;` after `GenerateSphere();`.
- **Trade-offs**: None.
- **Priority**: Critical.

### 0.2 Sphere generation (robustness) — done
- **Correction after review**: the fixed arrays `[7803]`/`[15000]` *did* match the 51×51
  vertex / 50×50×6 index counts exactly, and positions were fully written — so the sphere
  was correct for position-only rendering once 0.1 (the `break`) was in place. The real
  risk was the magic-number arrays overflowing if the segment/stack counts ever change.
- **Done**: generator now writes straight into the member `std::vector`s (position-only,
  stride 3), sized from the params. Output is identical for the current 50/50 settings.
- **Future**: to make the sphere lit/textured, emit the 11-float layout with normals
  (= normalized position for a unit sphere). Deferred — see DECISIONS open question 1.
- **Priority**: Critical.

### 0.3 `Texture::unit` uninitialized
- **Problem**: Used in `Bind()`, never set.
- **Proposal**: Store the intended unit (derive from `slot - GL_TEXTURE0`, or pass it in)
  in the ctor; initialize members in-class.
- **Trade-offs**: None.
- **Priority**: Critical.

### 0.4 Object camera-matrix uploaded after draw
- **Problem**: In the object pass, `camera.Matrix(shaderProgram, "camMatrix")` is called
  **after** `glDrawElements`, so the draw uses a stale matrix (one-frame lag; undefined
  first frame).
- **Proposal**: Move the upload before the draw (right after `updateMatrix`/`Inputs`).
- **Trade-offs**: None; removes visible lag/jitter.
- **Priority**: Critical.

---

## Phase 1 — Resource safety (High)
Foundation for everything else.

### 1.1 RAII + non-copyable GL wrappers — ✅ DONE (2026-11-19, Debug|x64 builds)
- **Was**: Wrappers had no destructors and were copyable over raw handles; cleanup was
  manual and incomplete (light VAO/VBO/EBO and the texture were never deleted). Copy-by-
  value of `VerTexBuffer` into `LinkAttriBute` was a latent double-free.
- **Done**: `VerTexArray`, `VerTexBuffer`, `Triangle`, `Shader`, `Texture` now have
  destructors that free the handle, deleted copy ctor/assignment, and move ctor/assignment
  (transfer handle, null the source). `Delete()` is idempotent (`if(ID) ... ID=0`) and the
  destructor calls it. `LinkAttriBute` takes `VerTexBuffer&` (no copy). Handles default to
  `0`; `glDelete*` on 0 is a safe no-op.
- **main.cpp**: all GL wrappers were moved into an explicit scope block so their
  destructors run **before** `glfwDestroyWindow`/`glfwTerminate` (context still current);
  the manual `Delete()` block was removed. This also fixes the previously leaked light
  sphere VAO/VBO/EBO and the texture.
- **Trade-off accepted**: objects now have fixed lifetimes tied to that block — correct and
  conventional.

### 1.2 GL error checking + graceful asset-load failure — ✅ DONE (2026-11-19)
- **Done**: added `GLUtils.h` with `glCheckError()` (drains `glGetError`, logs code +
  file:line; no KHR_debug dependency, works on the 3.3 context). Called after buffer
  creation (`VerTexBuffer`, `Triangle`), attribute linking (`VerTexArray`), and texture
  creation (`Texture`).
- **Graceful failures**: `get_file_contents` now throws `std::runtime_error` with the
  missing path (was `throw(errno)` → abort). `Texture` throws if `stbi_load` returns null.
  `main`'s GL section is wrapped in try/catch that logs `[FATAL] ...` and exits -1; RAII
  unwinding frees any already-created GL objects while the context is current.
- **Note**: this is the robust counterpart to the asset-copy build step — if assets are
  genuinely missing the app now reports which file instead of aborting.
- **Future (optional)**: upgrade to `glDebugMessageCallback` if the context is raised to
  4.3+ for automatic, richer messages.

---

## Phase 2 — Structure & clarity (Medium)

### 2.1 Fix misleading names — ✅ DONE (2026-11-19, clean rebuild)
- **Done**: `Triangle` class → **`ElementBuffer`** (files `Triangles.{h,cpp}` →
  `ElementBuffer.{h,cpp}`, vcxproj + filters updated). `VerticesStride()`/`TriangleStride()`
  → **`VertexBytes()`/`IndexBytes()`** (they return byte sizes, not strides).
  `SeUniFormValueMattrix` → **`SetUniformMat4`**. All call sites in `main.cpp` updated.
- **Left as-is (stylistic, not misleading)**: `LinkAttriBute`, `SetUniformValueF/Vec3/Vec4`,
  `GetUniFormId` — odd casing but unambiguous; renaming is pure churn. Revisit only if a
  broader API cleanup happens.

### 2.2 Extract a minimal `Mesh`/draw helper — ✅ DONE (2026-11-19)
- **Done**: `Mesh.{h,cpp}` owns VAO+VBO+EBO (RAII members → non-copyable, movable) and
  takes `(vertices, indices, layout)` where layout is a `std::vector<VertexAttribute>`;
  it computes stride/offsets and exposes `Draw()` (binds its VAO + `glDrawElements`).
  `main` now builds `pyramidMesh` (pos/color/uv/normal) and `sphereMesh` (pos) and calls
  `.Draw()`; the manual VAO/VBO/EBO plumbing and raw `glDrawElements` calls are gone.
- **Kept thin on purpose**: shader/texture binding stays in `main`; no scene graph. The
  EBO is bound while the VAO is bound in the Mesh ctor so the VAO records it.

### 2.3 Delta-time-correct camera — ✅ DONE (2026-11-19)
- **Was**: `Camera::speed` not scaled by `tick`; movement frame-rate dependent; `tick`
  ignored. (Same class of bug as the pyramid spin fixed in `main.cpp`.)
- **Done**: movement now multiplies `speed * tick`; `speed` redefined as units/second
  (3.0 normal, 8.0 sprint). Sprint check moved before movement so the correct speed is
  used the same frame. Mouse sensitivity unchanged (already pixel-delta based).
- **Related**: pyramid rotation in `main.cpp` was also made frame-rate independent
  (rebuild model from identity each frame, advance angle by `20 * dt` deg) — it was
  compounding every frame and spinning violently at high FPS.

### 2.4 Shader correctness — ✅ DONE (2026-11-19)
- **Done**: `Light.frag` now writes the declared `out vec4 FragColor` instead of deprecated
  `gl_FragColor`. `default.vert` uses `transpose(inverse(mat3(model)))` for the normal
  matrix (correct under non-uniform scale; the old code also multiplied on the wrong side).
  `Shader::CompileError` uses `strcmp` instead of comparing `const char*` pointers, and the
  PROGRAM branch checks `GL_LINK_STATUS` (was `GL_COMPILE_STATUS`).
- `_Time` uniform in `Light.frag` left in place (harmless, still set from main; a future
  cleanup could remove it or animate the light with it).
- **Verify**: lit pyramid should look unchanged (model is rotation-only).

---

## Phase 3 — Extensibility (Low, optional / future)

### 3.1 Text rendering with FreeType — ✅ DONE (2026-11-19)
- **Done**: FreeType built from source (VER-2-13-3, static, optional deps disabled) and
  vendored into `Libraries/lib/freetype.lib` (+ `freetyped.lib` for Debug) and headers
  under `Libraries/source/{ft2build.h,freetype/}`. Linked per-config in the vcxproj.
- `Text.{h,cpp}` is now a real renderer: rasterizes the first 128 ASCII glyphs into one
  texture each, draws textured quads with an orthographic pixel-space projection,
  alpha-blended with depth test disabled. `main` shows an on-screen FPS HUD.
- Text shaders: `Shader/text.vert`, `Shader/text.frag`. Font: `./Fonts/arial.ttf`
  (NOT committed — see note). zlib left vendored/unused (FreeType built without it).
- **Font note**: `OpenGlCore/Fonts/*.ttf` is gitignored to avoid committing a proprietary
  font; a local copy of Arial is used. Supply any `.ttf` as `./Fonts/arial.ttf`, or change
  the path in `main.cpp` where `Text hud(...)` is constructed.

### 3.2 Model loading with Assimp — ✅ DONE (2026-11-19)
- **Done**: Assimp added via vcpkg manifest (`vcpkg.json`, dynamic). `Model.{h,cpp}` loads
  a file (FBX/OBJ/glTF) with `Triangulate | GenSmoothNormals | FlipUVs`, flattens each
  Assimp mesh into this project's 11-float layout (pos/color/uv/normal) and stores them as
  `Mesh` objects, so a model draws with the existing `default.*` shaders. `Model::Draw()`
  draws all sub-meshes. `main` loads `./Models/Cube.fbx` and draws it beside the pyramid.
- **Build wiring**: vcxproj adds the vcpkg include/lib dirs per config and links
  `assimp-vc145-mt[d].lib`; a post-build target copies the Assimp DLLs next to the exe.
- **Rendering**: models draw with a dedicated albedo shader (`default.vert` +
  `albedo.frag`) — lit solid base color via an `albedo` uniform, no texture. `Model`
  computes an AABB (`Size()`/`Center()`/`ScaleToFit(target)`); `main` scales the cube to
  ~2 units and recenters it, so native FBX units don't matter.
- **Node hierarchy**: `Model` now walks the `aiNode` tree, accumulates each node's
  transform and bakes it into the mesh vertices (positions + inverse-transpose for
  normals), so multi-part FBX files are positioned correctly. Bounds are computed from the
  transformed positions.
- **Materials**: per sub-mesh diffuse color is imported (`AI_MATKEY_COLOR_DIFFUSE`).
- **Textures**: per sub-mesh diffuse texture is imported — external files (resolved against
  the model's folder) and embedded textures (compressed via stb, or raw BGRA texels). The
  model shader (`albedo.frag`) branches on a `hasTexture` uniform: textured sub-meshes
  sample `modelTex` (unit 0), untextured ones use the albedo color. `Model` owns the GL
  textures and frees them in its destructor.
- **Still not handled**: normal/specular/PBR maps (diffuse only), and skinning/animation
  (bind pose only).

### 3.3 Platform decoupling — ✅ DONE (2026-11-19)
- **Done**: removed `Windows.h`/`WinUser.h`; monitor size now comes from
  `glfwGetVideoMode(glfwGetPrimaryMonitor())`. Added a `FULLSCREEN` compile-time flag
  (windowed 1280x720 when false). This is the only Win32 coupling that existed; the code
  is now platform-neutral at the source level (build is still a VS/MSBuild project).

---

## Migration order (summary)
`0.1 → 0.2 → 0.3 → 0.4` (independent, do together) → `1.1 → 1.2` → `2.x` → `3.x`.

## Risks
- **1.1 (RAII)** is the highest-risk change: introducing destructors turns today's silent
  handle copies into real double-frees, so copies **must** be eliminated in the same
  change. Validate by running after each wrapper is converted.
- Renames (2.1) risk missed call sites — rely on the compiler (no test suite exists).
- No automated tests: every phase is verified by **building and running** the app and
  visually confirming the pyramid (and, after 0.2, the sphere) render correctly.
