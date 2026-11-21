# Material System

Status: **asset format + resolve implemented & validated headlessly**. The Material Inspector,
preview and Project-browser "Create → Material" are layered in the editor (see docs/EDITOR.md).

## Runtime vs asset
- `PbrMaterial` (render/PbrMaterial.h) is the **runtime** material: live GL texture handles +
  scalar factors, bound to the PBR shader each draw. Not persistent.
- `MaterialAsset` (render/MaterialAsset.{h,cpp}) is the **persistent** `.mat` asset: stable data
  only — a GUID, name, shader id, scalar PBR properties, texture references **by GUID**, UV
  tiling/offset and a couple of render flags. Reconstructible after an editor restart; survives
  moving/renaming its textures because texture refs are GUIDs resolved through the AssetDatabase.

## `.mat` file format (JSON, versioned)
```json
{
  "kind": "Material", "version": 1,
  "guid": "<32 hex>", "name": "...", "shader": "pbr",
  "albedo": [r,g,b], "metallic": 0.0, "roughness": 0.5, "ao": 1.0,
  "albedoTex": "<guid|null>", "normalTex": "...", "metallicTex": "...",
  "roughnessTex": "...", "aoTex": "...",
  "tiling": [1,1], "offset": [0,0], "doubleSided": false
}
```
No GL handles or absolute paths are ever written.

## Resolving to a renderable material
`MaterialAsset::Resolve(AssetDatabase&, AssetManager&)` copies the scalars and resolves each
texture GUID → absolute path (via the database) → shared GL handle (via the AssetManager, which
caches so shared materials upload each texture once). `CaptureFrom(PbrMaterial&, db)` does the
reverse, mapping a runtime material's texture *paths* back to GUIDs to "extract" an asset.

## Validation
`OGL_SCENE_SELFTEST`: a MaterialAsset is written and reloaded with its GUID and scalar fields
intact (`guidStable=1`), and it participates in the asset-database rename test.

## Known limitations / next steps
- `tiling`/`offset` are stored and serialized but the PBR shader does not yet apply them (needs a
  UV-transform uniform); `doubleSided` is stored but not yet wired to face-culling state.
- Only the built-in `pbr` shader is selectable today.
- Material preview uses the existing PBR/IBL renderer (no separate pipeline).
