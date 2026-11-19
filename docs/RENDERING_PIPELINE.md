# Rendering Pipeline

> Current implementation as written in `main.cpp` + the GLSL in `OpenGlCore/Shader/`.

## Setup (once, before the loop)
1. `glfwInit`, hint GL **3.3 core**, create **fullscreen** window on primary monitor.
2. `glfwMakeContextCurrent`, `gladLoadGL`, `glViewport(0,0,WIDTH,HEIGHT)`.
3. Build `Shader shaderProgram` from `default.vert` + `default.frag`.
4. Pyramid: `VerTexArray VAO1` + `VerTexBuffer VBO1` + `Triangle EBO1`; link 4 attributes
   (pos/color/uv/normal) at stride `11*sizeof(float)`.
5. Build `Shader lightShader` from `Light.vert` + `Light.frag`; sphere VAO/VBO/EBO with a
   **single** position attribute at stride `3*sizeof(float)`.
6. Light transform: `lightModel = translate(I, lightPos)`, `lightPos = (2.5, 2, 0)`.
7. Load `Images/Brick.jpg` into `Texture`, bind sampler `tex0` to unit 0.
8. Upload static uniforms: `lightPos`, `lightcolor`/`LightColor`, light `model`, `Scale`.
9. `Camera camera(..., pos=(0,0.5,2))`; `glEnable(GL_DEPTH_TEST)`.

## Per-frame loop
1. Timing: compute `timeDiff`; update window title with FPS/ms (threshold `1.0/1000`,
   so effectively every frame).
2. `Time::_DeltaTime = now - lastFrame`; advance `rotation` (clamped to [0,1]).
3. `model = rotate(model, radians(rotation), yAxis)` — **accumulates** each frame (model
   is not reset), so rotation compounds.
4. Clear color + depth.
5. **Light pass**: `lightShader.Activate` → `camera.Matrix(lightShader,"camMatrix")` →
   bind sphere VAO → upload light `model`, `_Time` → `glDrawElements(sun.Indices.size())`.
6. **Object pass**: `shaderProgram.Activate` → `camera.updateMatrix(FOV,0.3,100)` →
   `camera.Inputs(window, dt)` → set `scale=1` → bind `VAO1` + texture → upload `model`
   → `glDrawElements(p.Indices.size())` → **then** `camera.Matrix(shaderProgram,...)`.
7. `glfwSwapBuffers`, `glfwPollEvents`.

### Ordering issues (current, confirmed)
- **Object camera matrix is uploaded *after* `glDrawElements`** (step 6). The draw uses
  the *previous* frame's `camMatrix`; first frame uses whatever the shader defaults to.
- `shaderProgram` uniforms `lightPos`/`lightcolor`/`CamPos` are set while *different*
  programs may be active at various points; `CamPos` is set near top of loop before the
  light pass activates `lightShader`. Verify active-program vs. uniform-target when
  touching this code.

## Shaders

### `default.vert` / `default.frag` (lit object)
- Vertex: builds `MVP = camMatrix*model`, passes a `Data` struct (world Position, Normal,
  color, UV). `gl_Position = MVP * vec4(aPos*scale,1)`.
- Normal transform is `normalize(Normal * transpose(mat3(model)))` — note it uses the
  transpose of the model's upper-3x3, **not** the inverse-transpose; correct only for
  pure rotations/uniform scale. Fine today (rotation only), fragile under non-uniform
  scale.
- Fragment: ambient(0.2) + diffuse (Lambert) + specular (pow 8), modulated by
  `texture(tex0,uv) * lightcolor`. Uses GLSL 330.

### `Light.vert` / `Light.frag` (light source)
- Vertex: `gl_Position = camMatrix*model*vec4(aPos*Scale,1)`.
- Fragment: writes `gl_FragColor = LightColor`. **Uses deprecated `gl_FragColor`** while
  declaring an unused `out vec4 FragColor`; `_Time` uniform is unused. Works on most
  drivers in 330 compat but is non-core-correct.

## Vertex layout (lit objects)
`[ position.xyz | color.rgb | uv.xy | normal.xyz ]` = 11 floats/vertex.
Light sphere uses position-only (3 floats) — but its generator is broken; see Roadmap.
