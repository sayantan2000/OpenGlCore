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

### 2.1 Fix misleading names (opt-in, breaking)
- **Problem**: `Triangle` is an EBO; `VerticesStride()`/`TriangleStride()` return byte
  sizes; `SeUniFormValueMattrix` and other typo method names.
- **Proposal**: Rename to `ElementBuffer`, `VertexBytes()`/`IndexBytes()`, corrected
  uniform setter names — **in one pass**, updating all call sites. Keep thin aliases only
  if needed for a transition.
- **Trade-offs**: Touches many files; purely mechanical. Do as its own commit.
- **Priority**: Medium.

### 2.2 Extract a minimal `Mesh`/draw helper
- **Problem**: `main` manually binds VAO/texture/shader and calls `glDrawElements` with
  hand-maintained strides and index counts.
- **Proposal**: A small `Mesh` that owns VAO+VBO+EBO and exposes `Draw()`; the 11-float
  attribute layout defined in one place. Keep it thin — no scene graph.
- **Trade-offs**: A little abstraction; reduces `main` duplication for N objects.
- **Priority**: Medium. **Depends on**: 1.1.

### 2.3 Delta-time-correct camera — ✅ DONE (2026-11-19)
- **Was**: `Camera::speed` not scaled by `tick`; movement frame-rate dependent; `tick`
  ignored. (Same class of bug as the pyramid spin fixed in `main.cpp`.)
- **Done**: movement now multiplies `speed * tick`; `speed` redefined as units/second
  (3.0 normal, 8.0 sprint). Sprint check moved before movement so the correct speed is
  used the same frame. Mouse sensitivity unchanged (already pixel-delta based).
- **Related**: pyramid rotation in `main.cpp` was also made frame-rate independent
  (rebuild model from identity each frame, advance angle by `20 * dt` deg) — it was
  compounding every frame and spinning violently at high FPS.

### 2.4 Shader correctness
- **Proposal**: In `Light.frag` use the declared `out vec4 FragColor` instead of
  deprecated `gl_FragColor`; remove unused `_Time` or use it. In `default.vert` use the
  proper normal matrix `transpose(inverse(mat3(model)))` (or pass it as a uniform) so
  non-uniform scale is correct. Fix `Shader::CompileError` string comparison (`strcmp`,
  not `!=` on `const char*`).
- **Trade-offs**: None meaningful; more correct lighting under scaling.
- **Priority**: Medium.

---

## Phase 3 — Extensibility (Low, optional / future)

### 3.1 Decide freetype + zlib fate
- **Problem**: Both vendored but unused; `Text` is an empty stub.
- **Proposal**: Either (a) implement `Text` with freetype (glyph atlas + textured quads)
  and remove zlib if not needed, or (b) drop the unused vendored libs to shrink the repo.
  **Do not** wire them in silently.
- **Priority**: Low.

### 3.2 Model loading
- **Problem**: `Model::Load` empty; `Cube.fbx` unused. FBX needs a heavy dependency.
- **Proposal**: If model loading is a goal, prefer a light format (OBJ/glTF) + a small
  loader over FBX/Assimp for a learning project. Defer until needed.
- **Priority**: Low.

### 3.3 Platform decoupling
- **Problem**: `Windows.h`/`GetSystemMetrics` for monitor size; windowed-vs-fullscreen
  hard-coded.
- **Proposal**: Use GLFW monitor/video-mode APIs (`glfwGetVideoMode`) instead of Win32;
  make fullscreen a flag.
- **Priority**: Low.

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
