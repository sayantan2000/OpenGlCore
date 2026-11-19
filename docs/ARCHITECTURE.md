# Architecture

> Describes the **actual current implementation**. Proposed changes live in
> `docs/ROADMAP.md`.

## High-level shape
There is no engine abstraction. `main.cpp` is the application: it owns setup, the render
loop, and teardown. Everything else is a **thin wrapper** over one or two GL objects, or
a utility/stub. All orchestration (binding, uniform upload, draw order) is hand-written
in `main()`.

```
main.cpp ── creates/drives ──> GLFW window + GL context (GLAD)
   │
   ├─ Primitive        geometry source (CPU vertex/index data)
   ├─ VerTexBuffer     VBO wrapper
   ├─ Triangle         EBO wrapper (misnamed: it is an Element Buffer Object)
   ├─ VerTexArray      VAO wrapper + attribute linking
   ├─ Shader           program: compile/link/activate/uniforms
   ├─ Texture          2D texture load (stb) + bind
   ├─ Camera           view/projection matrix + input
   └─ Time             static delta-time holder
```

## Module reference

| File | Role | Notes / debt |
|------|------|------|
| `main.cpp` | App entry, setup, render loop, teardown | Monolithic; hard-coded layout strides; manual bind/draw. |
| `Primitive.{h,cpp}` | CPU geometry (pyramid hard-coded array; sphere generated) | Phase 0 fixed the switch fall-through and sphere generator (now vector-based, position-only). Sphere still has no normals/UVs (fine for the light marker). Byte-size accessors renamed to `VertexBytes()`/`IndexBytes()` (Phase 2.1). |
| `VerTexArray.{h,cpp}` | VAO + `LinkAttriBute` | RAII (Phase 1.1): non-copyable/movable; `LinkAttriBute` takes `VerTexBuffer&`. |
| `VerTexBuffer.{h,cpp}` | VBO | RAII (Phase 1.1): destructor frees handle; non-copyable, movable. |
| `ElementBuffer.{h,cpp}` | EBO (class `ElementBuffer`) | Renamed from `Triangle`/`Triangles.*` (Phase 2.1). RAII like the other buffers. |
| `Shader.{h,cpp}` (`shaderClass`) | Program build + uniforms | `type != "PROGRAM"` compares `const char*` pointers, not strings (works by luck via string literal pooling). Methods have typo names used by callers. |
| `Texture.{h,cpp}` | stb image load → GL texture | **`unit` member never initialized**, yet used in `Bind()`. Internal format hard-coded `GL_RGBA`. |
| `Camera.{h,cpp}` | View/projection + WASD/mouse | `speed` not scaled by delta-time (frame-rate dependent); `tick` param unused. |
| `Time.{h,cpp}` | Static `_DeltaTime` etc. | Global mutable state set from `main`. |
| `Text.{h,cpp}` | Text rendering | **Empty stub.** |
| `Model.{h,cpp}` | Model loading | **Empty stub** (`Load` does nothing). |
| `Enums.h` | `PrimitiveType` | `static enum` (unscoped + `static` is odd); values unscoped (`sphere`, `pyramid`…). |
| `glad.c`, `stb.cpp` | Third-party single-file impls | Leave as-is. |

## Dependencies
- **GLFW** (`glfw3.lib`) — windowing/input. **GLAD** — GL function loader.
- **GLM** — math (header-only). **stb_image** — texture loading.
- **opengl32.lib** — system GL.
- **freetype, zlib** — vendored, **unused** by project code today.
- **Windows.h** — `GetSystemMetrics` for monitor size (ties project to Windows).

## Include/link model
- VS project adds `Libraries/source` to include path and `Libraries/lib` to lib path.
- Headers referenced as `<glad/glad.h>`, `<GLFW/glfw3.h>`, `<glm/...>`, `<stb/stb.h>`.

## Data flow (one lit object)
1. `Primitive` fills `std::vector<GLfloat> vertices` + `std::vector<GLuint> Indices`.
2. `VerTexBuffer` uploads vertices; `Triangle` uploads indices.
3. `VerTexArray::LinkAttriBute` sets 4 attributes at stride `11*sizeof(float)`.
4. Per frame: `Shader::Activate`, set uniforms, `Camera::updateMatrix` + upload,
   `Texture::Bind`, `glDrawElements`.

See `docs/RENDERING_PIPELINE.md` for the exact per-frame sequence and its ordering bug.
