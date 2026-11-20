#ifndef EDITOR_H
#define EDITOR_H

#include <string>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include "EditorCommand.h"
#include "PrimitiveLibrary.h"

struct GLFWwindow;
class Scene;
class Camera;
class AssetManager;
class Input;
class ScriptSystem;

// Per-frame render settings the Inspector/menus can tweak.
struct RenderSettings
{
	float exposure = 1.0f;
	float skyboxIntensity = 1.0f;
	bool  useIBL = true;
	bool  showSkybox = true;
	float fps = 0.0f;
};

// Dear ImGui editor: a Unity/Unreal-style editor with a FIXED, non-movable layout
// (Hierarchy left, Scene Viewport centre, Inspector right, Console bottom). Adds viewport
// object picking (ray/AABB), transform gizmos (ImGuizmo), keyboard shortcuts, camera focus,
// a Hierarchy create context menu, primitive/light creation, and a centralized undo/redo
// command history. All edits operate on the live Scene.
class Editor
{
public:
	enum class GizmoOp { Translate, Rotate, Scale };
	enum class GizmoSpace { World, Local };

	void Init(GLFWwindow* window, Camera* camera, const PrimitiveLibrary* primitives,
		AssetManager* assets, Input* input, ScriptSystem* scripts);
	void Shutdown();

	void BeginFrame();                       // ImGui + ImGuizmo new frame
	// Builds panels and handles all viewport interaction for this frame.
	void BuildUI(Scene& scene, RenderSettings& settings,
		const glm::mat4& view, const glm::mat4& proj, int fbW, int fbH, float dt);
	void Update(Scene& scene, float dt);     // runs scripts while in Play mode
	void EndFrame();                         // render draw data

	bool IsPlaying() const { return isPlaying; }

	// True while ImGui is using the mouse/keyboard (camera should ignore input).
	bool WantCaptureMouse() const;
	bool WantCaptureKeyboard() const;
	// True while the gizmo is hovered or being dragged (camera should ignore mouse-look).
	bool WantCaptureViewport() const;

	void Log(const std::string& msg);

private:
	// Panels
	void DrawMainMenuBar(Scene& scene);
	void DrawHierarchy(Scene& scene);
	void DrawHierarchyNode(Scene& scene, GameObject* go);
	void Reparent(Scene& scene, std::uint32_t childId, std::uint32_t parentId);
	void DrawInspector(Scene& scene, RenderSettings& settings);
	void DrawConsole(const RenderSettings& settings);
	void DrawHistoryPanel();

	// Viewport interaction
	void HandleCameraNavigation(Scene& scene); // scroll-zoom, Alt+LMB orbit, MMB pan
	void HandlePicking(Scene& scene, const glm::mat4& view, const glm::mat4& proj);
	void DrawGizmo(Scene& scene, const glm::mat4& view, const glm::mat4& proj);
	void HandleShortcuts(Scene& scene);
	void FocusSelected(Scene& scene);

	// Scene operations (all undoable)
	void CreateEmpty(Scene& scene);
	void CreatePrimitive(Scene& scene, PrimitiveType type);
	void CreateLight(Scene& scene, int lightType); // LightComponent::Type as int
	void DuplicateSelected(Scene& scene);
	void DeleteSelected(Scene& scene);
	GameObject* Commit(Scene& scene, std::unique_ptr<GameObject> go, const char* label);

	// Scene file I/O (JSON). Clears undo history on new/load (ids change).
	void NewScene(Scene& scene);
	void SaveScene(Scene& scene);
	void LoadScene(Scene& scene);

	// Play mode: snapshot the scene, run scripts, restore on stop.
	void EnterPlayMode(Scene& scene);
	void ExitPlayMode(Scene& scene);

	// Inspector change tracking -> one undo command per interaction.
	void TrackEdit(Scene& scene, GameObject* go, const char* label);

	// State
	bool initialized = false;
	std::vector<std::string> console;
	Camera* camera = nullptr;
	const PrimitiveLibrary* primitives = nullptr;
	AssetManager* assets = nullptr;
	Input* input = nullptr;
	ScriptSystem* scripts = nullptr;
	CommandHistory history;
	std::string scenePath = "scene.json"; // default save/load path (cwd)

	bool isPlaying = false;
	std::string playSnapshot; // scene JSON captured on Play, restored on Stop

	GizmoOp gizmoOp = GizmoOp::Translate;
	GizmoSpace gizmoSpace = GizmoSpace::World;
	bool showHistoryPanel = false;

	// Layout (computed each frame from the display size).
	glm::vec4 viewportRect = glm::vec4(0.0f); // x, y, w, h (pixels)
	int fbWidth = 0, fbHeight = 0;

	// Inspector edit tracking.
	bool editing = false;
	GameObject editBefore;
	std::uint32_t editId = 0;

	// Gizmo drag tracking.
	bool gizmoWasUsing = false;
	GameObject gizmoBefore;
};

#endif
