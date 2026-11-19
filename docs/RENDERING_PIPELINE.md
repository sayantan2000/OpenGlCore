# Rendering Pipeline

> Current implementation (`Application.cpp` + `IBL.cpp` + `OpenGlCore/Shader/`).

## Startup
1. `Window` creates the GL 3.3 core context (GLFW/GLAD), enables depth test.
2. `Application` builds: `Camera`, `pbr`/`hdr` shaders, `IBL`, the sphere `Mesh`, the
   `Model`, the `Text` HUD, the `LightSet`, and the HDR framebuffer.
3. **IBL build** (`IBL` ctor, one-time, into FBOs then viewport restored):
   - **Environment source**: load an equirectangular HDR image into a `RGB16F` texture,
     preserving HDR values and flipped vertically — `.exr` via **tinyexr** (zlib-backed),
     `.hdr` via stb `stbi_loadf`. Render it onto the cube faces with `equirect.frag`
     (direction → lat/long UV: `uv = (atan(z,x), asin(y)) * (1/2pi, 1/pi) + 0.5`). If the
     file is missing, fall back to the **procedural sky** (`sky.frag`). Either way the result
     fills a 512³ `RGB16F` env cubemap (6 faces via `cubemap.vert` + the standard capture
     projection/views); mipmaps generated. The equirect source texture is freed after
     capture (only the cubemap is kept). Default asset: `./SKYBOX/ballroom_4k.exr`.
   - Convolve it into a 32³ **irradiance** cubemap (`irradiance.frag`).
   - GGX-importance-sample it into a 128³ **prefiltered** specular cubemap with 5 mips
     (`prefilter.frag`), roughness mapped to mip level.
   - Integrate the **BRDF LUT** into a 512² `RG16F` 2D texture (`hdr.vert` + `brdf.frag`).

## Per frame
1. Timing: delta time; FPS → title + HUD (every 0.5 s).
2. Camera input + `updateMatrix(fov=60, near=0.1, far=200)`.
3. **Scene pass → HDR framebuffer** (`RGBA16F`), `RenderSceneHDR()`:
   - Activate `pbr`; set `u_camPos`, `camMatrix`; upload lights (`LightSet::Upload`).
   - Bind IBL: irradiance→unit 0, prefilter→unit 1, BRDF LUT→unit 2; `u_useIBL=1`.
   - Draw the 7×7 sphere grid: per-sphere `u_metallic` (row), `u_roughness` (col), shared
     `u_albedo`; map flags off (scalar material).
   - Draw the model via `Model::DrawPBR` (albedo color/texture, dielectric defaults).
   - Draw the **skybox** (`background.*`) sampling the env cubemap (same cubemap the IBL
     uses — no duplication), view rotation only (translation dropped) so it stays centered
     on the camera, depth `LEQUAL` with depth forced to 1.0 so it fills only un-written
     pixels. `u_bgIntensity` scales skybox brightness independently of the HDR exposure.
4. **Tonemap pass → default framebuffer** (`TonemapToScreen()`): fullscreen triangle
   (`hdr.vert`, no VBO), `hdr.frag` samples the HDR color, applies ACES + exposure, then
   linear→sRGB gamma.
5. **HUD**: `Text::RenderText` (2D overlay, alpha-blended, depth off).
6. Swap + poll.

## PBR shading (`pbr.frag`)
Cook-Torrance specular BRDF:
- **D** = GGX / Trowbridge-Reitz (`DistributionGGX`).
- **G** = Smith with Schlick-GGX, direct-lighting `k = (r+1)²/8`.
- **F** = Schlick (`fresnelSchlick`); roughness-aware variant for IBL.
- Metallic workflow: `F0 = mix(0.04, albedo, metallic)`; `kD = (1-F)(1-metallic)` for
  energy conservation; Lambertian diffuse `albedo/π`.
- Lights: directional (parallel), point and spot with **inverse-square** attenuation; spot
  adds a smooth cone from inner/outer cosines.
- Ambient: IBL = `kD · irradiance · albedo + prefiltered · (F·brdf.x + brdf.y)`, scaled by
  AO; falls back to `0.03·albedo·ao` when `u_useIBL=0`.
- Maps (when present, units 3–7): albedo (sRGB→linear), normal (perturbed via screen-space
  derivatives — no precomputed tangents), metallic, roughness, AO.
- Output is **linear HDR**; tonemapping happens in the HDR pass, not here.

## Color space
Albedo textures are decoded sRGB→linear in the shader; lighting is done in linear space;
the final HDR pass applies ACES then the linear→sRGB (1/2.2) gamma. No sRGB framebuffers.

## Vertex layout & texture units
See `CLAUDE.md` — 11-float layout; units 0–2 IBL, 3–7 material maps.
