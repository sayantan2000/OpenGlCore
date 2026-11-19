# Refactoring Roadmap

> **Status: PROPOSED — not approved.** No source refactoring has been performed. Execute
> items only after the maintainer approves them. Order respects dependencies. Each item
> lists: Problem → Why → Proposal → Trade-offs → Priority.
>
> Priorities: **Critical** (broken/incorrect behavior) · **High** (safety/architecture
> foundations) · **Medium** (quality/extensibility) · **Low** (polish).

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
