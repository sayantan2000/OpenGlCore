# Project System & Engine Hub

Status: **implemented & validated headlessly** (Hub project scaffolding/registry via
`EngineHub.exe --selftest`; editor `--project` handoff + scene persistence via the engine
self-test). The Hub GUI itself requires a display and was not visually verified here.

## Two processes
- **Engine Hub** (`Hub/`, builds `EngineHub.exe`): a standalone application that manages projects
  and launches the editor. Independent of the engine runtime — it links only GLFW + Dear ImGui +
  OpenGL + the vendored JSON header (no renderer, physics or asset-loading code).
- **Editor** (`OpenGlCore.exe`): opens exactly one project, passed on the command line.

They communicate by process launch: the Hub runs `OpenGlCore.exe --project "<path>"` (quoted, so
spaces/special characters are safe) via `CreateProcessW`, with the child's working directory set to
the editor's own folder (where its `Shader/`, `Images/`, … live).

## Hub features (`Hub/main.cpp`, `ProjectTemplate`, `ProjectRegistry`)
- **Projects** tab: recent projects (most-recent-first), Open / Remove-from-list (never deletes
  files), and a `[missing]` marker for projects whose folder is gone.
- **New Project** tab: name + location + template (Blank 3D / Starter 3D), then **Create & Open** —
  scaffolds the project and launches the editor.
- **Open Project** tab: open an existing project folder by path (validated).
- **About** tab: engine version + registry file location.
- Recent projects persist to `%LOCALAPPDATA%/OpenGlCoreHub/projects.json` (`ProjectRegistry`).

## Project creation (`ProjectTemplate::CreateProject`)
Creates `<location>/<name>/` with the standard skeleton (see docs/ASSET_PIPELINE.md),
`ProjectSettings/project.json` (`name`, `engineVersion`, `template`, `created`), and an empty
`Assets/Scenes/Main.scene` placeholder. Refuses to create over a non-empty folder.

## Editor handoff & scene seeding
`main(argc, argv)` parses `--project <path>` (or `-p`, or `--project=<path>`) and constructs
`Application(projectPath)`. With no argument the editor opens/creates a local `Project/` folder so
there is always a valid project. On open:
- If `Assets/Scenes/Main.scene` exists → it is loaded.
- Otherwise the scene is seeded from the project's `template` (`blank` → empty; anything else →
  the built-in demo), then **saved** to `Main.scene` so the project persists its own scene.

## Validation
- `EngineHub.exe --selftest` → `[HUB-SELFTEST] PASS`: project creation, validity, scene/settings
  generation, name read-back, duplicate rejection, and registry add/remove.
- Engine self-test: opening `--project <tmp>` creates the project, seeds + persists its scene, and a
  second open loads the persisted scene (round-trip).

## Known limitations / next steps
- Location/path fields are text inputs (no native folder-picker dialog yet).
- No multi-editor-version management yet (single engine version string).
- The Hub does not yet show project thumbnails or last-opened times in the list beyond the path.
