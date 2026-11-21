# Asset Database & Project System

Status: **data layer implemented & validated headlessly** (GUIDs, `.meta` sidecars, project
skeleton, scan/refresh, move/rename reference preservation). Project-browser UI and full import
settings UI are layered on top in the editor (see docs/EDITOR.md).

## Project structure
A project is a folder containing:

```
Assets/        Scenes/ Materials/ Textures/ Models/ Shaders/ Scripts/ Audio/ Prefabs/ Plugins/
Library/       (import cache / derived data — not authored)
Temp/          (scratch)
Build/         (player builds)
ProjectSettings/
```

`AssetDatabase::CreateProjectSkeleton(root)` creates this tree; `Open(root)` ensures it then scans.

## GUIDs and `.meta` sidecars (`io/Guid.h`, `io/AssetDatabase.{h,cpp}`)
Every asset file gets a stable 128-bit **GUID** stored in a sibling `<file>.meta` (JSON: `guid`,
`type`, `importSettings`). Serialized references elsewhere (materials → textures, scenes → models)
store the **GUID**, never a path — so moving or renaming an asset does not break references: the
GUID travels with the file in its `.meta`, and a rescan simply updates the GUID→path mapping.

- `Refresh()` rescans `Assets/`: assigns GUIDs to new files (writing `.meta`), updates paths for
  files that moved, and refreshes modification times for reimport decisions. Idempotent.
- Lookups: `Find(guid)`, `FindByPath(rel)`, `PathToGuid`, `GuidToPath`, `GuidToAbsolutePath`.
- `InFolder(relFolder)` lists a folder's direct children for the Project browser.
- `RegisterNewAsset(rel)` indexes a file the engine just wrote (e.g. a material) without a full
  rescan.
- Type detection is by extension (`AssetTypeFromExtension`): Texture / Model / Shader / Audio /
  Material / Scene / Prefab / Script.

The database is the **index** only; the existing `AssetManager` still performs the actual GPU
loading/caching by path. The database is owned by the Application — not a singleton.

## Validation
`OGL_SCENE_SELFTEST` creates a throwaway project and checks:
`AssetDB opened=1 guidStable=1 renameStable=1` — skeleton created, a `.mat` round-trips with a
stable GUID, and after renaming the asset + its `.meta` the GUID is unchanged while the old path no
longer resolves.

## Known limitations / next steps
- Import settings are stored (`importSettings` object in `.meta`) but per-type import UIs and
  derived-data caching in `Library/` are not built yet; loading is still on-demand by path.
- No file-watcher; `Refresh()` is explicit (call it on focus / via the Project browser).
- Thumbnails are generated in the editor layer, not cached to `Library/` yet.
