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
| `Light.{h,cpp}` | Light structs + `LightSet::Upload(shader)` | `MAX_POINT_LIGHTS=8`, `MAX_SPOT_LIGHTS=4`. |
| `PbrMaterial.{h,cpp}` | Metallic-roughness material | Scalars + optional maps (units 3–7). |
| `IBL.{h,cpp}` | Builds irradiance/prefilter/BRDF-LUT from `sky.frag`; skybox | Owns cubemaps/LUT/cube VAO; frees in dtor. |
| `Model.{h,cpp}` | Assimp loader → `Mesh` list | Node transforms baked; material color + diffuse texture; `Draw` (albedo) / `DrawPBR`. |
| `Text.{h,cpp}` | FreeType glyph renderer | 2D overlay. |
| `Texture.{h,cpp}`, `Primitive.{h,cpp}` | Legacy (stb texture, pyramid/sphere data) | Not used by the PBR scene; kept for now. |
| `glad.c`, `stb.cpp` | Third-party single-file impls | Leave as-is. |

## Dependencies
- **GLFW** (window/input), **GLAD** (GL loader), **GLM** (math, header-only), **stb_image**
  (texture decode, incl. embedded + HDR-capable), all vendored under `Libraries/source`.
- **FreeType** — text (vendored prebuilt libs + headers in `Libraries/`).
- **Assimp** — model import (via vcpkg manifest `vcpkg.json`; restored to `vcpkg_installed/`).
- **opengl32.lib** — system GL. No Win32 dependency in source (GLFW handles the monitor).
- IBL uses a **procedural sky**, so no `.hdr`/HDR-image dependency.

## Design patterns (deliberately lightweight)
- **RAII** for every GL resource (rule-of-five wrappers; `unique_ptr` ownership in `Application`).
- **Facade**: `Application` is the one place the subsystems are composed.
- **Plain data + upload**: `LightSet`/`PbrMaterial` are POD-ish structs with an upload/bind
  method, not deep class hierarchies — avoids overengineering for a learning project.
