# Resource Management

> How GPU resources are currently created, owned, and destroyed. This is the weakest area
> of the codebase; see `docs/ROADMAP.md` for the fix plan.

## GPU resources in play
| Resource | Wrapper | Created | Destroyed |
|----------|---------|---------|-----------|
| VAO | `VerTexArray` | `glGenVertexArrays` in ctor | **destructor (RAII)** |
| VBO | `VerTexBuffer` | `glGenBuffers` + `glBufferData` in ctor | **destructor (RAII)** |
| EBO | `Triangle` | `glGenBuffers` + `glBufferData` in ctor | **destructor (RAII)** |
| Program | `Shader` | compile+link in ctor | **destructor (RAII)** |
| Texture | `Texture` | stb load + `glTexImage2D` in ctor | **destructor (RAII)** |

## Current ownership model: RAII (Phase 1.1, done 2026-11-19)
- Each wrapper (`VerTexArray`, `VerTexBuffer`, `Triangle`, `Shader`, `Texture`) acquires
  its GL handle in the constructor and frees it in its **destructor** (via an idempotent
  `Delete()`; handle defaults to `0`, `glDelete*(0)` is a no-op).
- Wrappers are **non-copyable** (deleted copy ctor/assignment) and **movable** (move
  transfers the handle and nulls the source). `LinkAttriBute` now takes `VerTexBuffer&`.
- `main` wraps all GL objects in a **scope block** that closes before
  `glfwDestroyWindow`/`glfwTerminate`, so destructors run while the context is current.
  The old manual `Delete()` calls were removed; the previously **leaked** light sphere
  VAO/VBO/EBO and the texture are now freed.

## Known resource bugs
1. ~~**`Texture::unit` uninitialized.**~~ **Fixed (Phase 0.3)**: the ctor now sets
   `unit = slot - GL_TEXTURE0`, so `Bind()` activates the correct unit.
2. **Texture internal format hard-coded `GL_RGBA`** while source may be RGB (`Brick.jpg`
   loaded with `format=GL_RGB`). Mismatch between internal format and supplied `format`
   is legal but wasteful/confusing; alpha is fabricated.
3. ~~**Sphere CPU buffers are fixed C arrays on the stack.**~~ **Fixed (Phase 0.2)**: the
   generator now writes straight into the member vectors sized from the params (positions
   only, stride 3). (The old arrays actually matched the counts, but would overflow if the
   segment/stack counts changed — that fragility is removed.)
4. **No GL error checking** around buffer/texture creation (shaders are the exception —
   they log compile/link errors).

## PBR / IBL / HDR resources (Phase 4)
- **`Application`** owns the HDR framebuffer (`hdrFBO` + `RGBA16F` color texture + depth
  RBO) and a small empty VAO for the fullscreen tonemap; all freed in `~Application`. The
  PBR/HDR `Shader`s, `IBL`, `Mesh`, `Model`, `Text` and `Camera` are held in `std::unique_ptr`
  so they are destroyed before `Window` (the GL context), which is the first member.
- **`IBL`** owns the environment/irradiance/prefilter cubemaps, the BRDF-LUT 2D texture, a
  cube VAO/VBO, and the background `Shader`; all freed in `~IBL`. Capture FBO/RBO are local
  to the constructor and deleted there; the window viewport is saved/restored around capture.
- **`Model`** owns any imported GL textures (freed in `~Model`); its `Mesh`es free their own
  VAO/VBO/EBO.

## Shader program lifecycle
- `Shader` ctor compiles both stages, links, logs errors via `CompileError`, deletes the
  intermediate shader objects. Good. `Delete()` frees the program; called for both
  programs at shutdown.

## Lifetime summary
All resources live for the whole program run (created before the loop, freed — partially —
after). There is no dynamic load/unload, no resource cache, no reference counting.
