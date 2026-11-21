# Editor

The Dear ImGui editor uses a FIXED layout: **Hierarchy** (left), **Scene Viewport** (centre, the
3D shows through), unified **Inspector** (right), and a tabbed **bottom panel** with **Console** and
**Project**. ImGuizmo gizmos, stencil outline, grid, camera navigation and undo/redo are unchanged
(see CLAUDE.md for controls).

## Project browser (bottom panel → Project tab)
Backed by the `AssetDatabase` (see docs/ASSET_PIPELINE.md) for the open project.

- Breadcrumb + **Up** navigates folders; **Refresh** rescans `Assets/`; **Create Material** writes
  a new `.mat` in the current folder (or `Materials/`) and opens it.
- Lists sub-folders (`[dir]`) and assets (`Type: name`). Double-click a folder to enter it;
  double-click a `.mat` to open it in the Material Inspector.
- Right-click a material: **Open in Material Inspector** or **Assign to Selected Object**.
- Selecting a **Texture** asset, then pressing **Set** on a texture slot in the Material Inspector,
  assigns that texture (by GUID) to the material.

## Material Inspector (right Inspector, when a `.mat` is open)
Edits the open `MaterialAsset` (see docs/MATERIAL_SYSTEM.md): name, albedo/metallic/roughness/AO,
five texture slots (GUID references with Set/Clear), UV tiling/offset and double-sided flag, plus
**Save** / **Reset** / **Close**. An **Assign to Selected Object** button applies the material
(resolved to a runtime `PbrMaterial`) to the selected mesh object as an undoable edit.

Material assignment is undoable (Ctrl+Z/Ctrl+Y) like any other object edit. Editing the material
asset's own fields writes to the `.mat` on Save (asset edits are not on the scene undo stack).

## Project handoff
The editor opens the project passed on the command line: `OpenGlCore --project <path>` (the Engine
Hub uses this). With no argument it opens/creates a local `Project/` folder so there is always a
valid project. See docs/PROJECT_SYSTEM.md.

## Known limitations
- Thumbnails/icons are text labels for now (no cached image thumbnails).
- Drag-and-drop of assets onto objects is not wired yet (use right-click → Assign).
- Live material **preview** on a sphere/cube/plane is not yet implemented in this panel (the
  material still renders correctly once assigned to a scene object).
- Folder create/rename/delete from the browser UI is not wired yet (use the filesystem /
  Create Material); the database picks up changes on Refresh.
