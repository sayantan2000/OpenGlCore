#include "Editor.h"
#include "Scene.h"
#include "GameObject.h"
#include "PbrMaterial.h"
#include "Camera.h"
#include "Picking.h"
#include "AssetManager.h"
#include "SceneIO.h"
#include "ScriptSystem.h"
#include "ScriptRegistry.h"
#include "Physics.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "ImGuizmo.h"

#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace ogl {

// ---------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------
namespace
{
	ImTextureID TexID(GLuint id) { return (ImTextureID)(intptr_t)id; }

	void DrawTexturePreview(const char* label, GLuint tex)
	{
		ImGui::Text("%s", label);
		ImGui::SameLine(160.0f);
		if (tex != 0) ImGui::Image(TexID(tex), ImVec2(48, 48));
		else ImGui::TextDisabled("(none)");
	}

	const char* kLightTypes[] = { "Directional", "Point", "Spot", "Rectangle (Area)" };

	// World-space AABB of a mesh object's local bounds under a world matrix (8 corners).
	void WorldBounds(const GameObject& go, const glm::mat4& m, glm::vec3& outMin, glm::vec3& outMax)
	{
		glm::vec3 lo = go.meshRenderer ? go.meshRenderer->localMin : glm::vec3(-0.5f);
		glm::vec3 hi = go.meshRenderer ? go.meshRenderer->localMax : glm::vec3(0.5f);
		glm::vec3 corners[8] = {
			{lo.x,lo.y,lo.z},{hi.x,lo.y,lo.z},{hi.x,hi.y,lo.z},{lo.x,hi.y,lo.z},
			{lo.x,lo.y,hi.z},{hi.x,lo.y,hi.z},{hi.x,hi.y,hi.z},{lo.x,hi.y,hi.z},
		};
		outMin = glm::vec3(1e30f); outMax = glm::vec3(-1e30f);
		for (auto& c : corners)
		{
			glm::vec3 w = glm::vec3(m * glm::vec4(c, 1.0f));
			outMin = glm::min(outMin, w);
			outMax = glm::max(outMax, w);
		}
	}
}

// ---------------------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------------------
void Editor::Init(GLFWwindow* window, Camera* cam, const PrimitiveLibrary* prims, AssetManager* am,
	Input* in, ScriptSystem* ss, PhysicsScene* ps)
{
	camera = cam;
	primitives = prims;
	assets = am;
	input = in;
	scripts = ss;
	physics = ps;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.IniFilename = nullptr; // fixed layout: don't persist/restore window positions
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	initialized = true;
	Log("Editor initialized (Dear ImGui + ImGuizmo).");
	Log("Nav: RIGHT-mouse fly (WASD) | scroll = zoom | Alt+LEFT = orbit selection | MIDDLE = pan.");
	Log("Left-click select | W/E/R move/rotate/scale | F focus | Ctrl+S/O save/load | Play/Stop.");
}

void Editor::Shutdown()
{
	if (!initialized) return;
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	initialized = false;
}

void Editor::BeginFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
}

void Editor::EndFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ---------------------------------------------------------------------------------------
// Main per-frame UI + interaction
// ---------------------------------------------------------------------------------------
void Editor::BuildUI(Scene& scene, RenderSettings& settings,
	const glm::mat4& view, const glm::mat4& proj, int fbW, int fbH, float /*dt*/)
{
	fbWidth = fbW;
	fbHeight = fbH;
	gridSpacing = settings.gridSpacing; // so DrawGizmo can snap translation to the grid

	DrawMainMenuBar(scene, settings);

	// Fixed layout computed from the display size (panels are non-movable / non-resizable).
	const ImGuiViewport* vp = ImGui::GetMainViewport();
	float menuH = ImGui::GetFrameHeight();
	float dispW = vp->WorkSize.x;
	float dispH = vp->WorkSize.y + vp->WorkPos.y - menuH; // area below the menu bar
	float top = menuH;
	const float leftW = 300.0f;
	const float rightW = 340.0f;
	const float consoleH = 190.0f;
	float centerH = dispH - consoleH;

	// Central viewport rect (the 3D scene shows through here).
	viewportRect = glm::vec4(leftW, top, dispW - leftW - rightW, centerH);

	const ImGuiWindowFlags fixedFlags =
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;

	// Hierarchy (left)
	ImGui::SetNextWindowPos(ImVec2(0, top), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(leftW, centerH), ImGuiCond_Always);
	ImGui::Begin("Scene Hierarchy", nullptr, fixedFlags);
	DrawHierarchy(scene);
	ImGui::End();

	// Inspector (right)
	ImGui::SetNextWindowPos(ImVec2(dispW - rightW, top), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(rightW, centerH), ImGuiCond_Always);
	ImGui::Begin("Inspector", nullptr, fixedFlags);
	DrawInspector(scene, settings);
	ImGui::End();

	// Console (bottom, full width)
	ImGui::SetNextWindowPos(ImVec2(0, top + centerH), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(dispW, consoleH), ImGuiCond_Always);
	ImGui::Begin("Console", nullptr, fixedFlags);
	DrawConsole(settings);
	ImGui::End();

	if (showHistoryPanel) DrawHistoryPanel();

	// Viewport interaction: gizmo first (so IsOver/IsUsing are valid), then picking, shortcuts.
	DrawGizmo(scene, view, proj);
	HandlePicking(scene, view, proj);
	HandleCameraNavigation(scene);
	HandleShortcuts(scene);
}

// ---------------------------------------------------------------------------------------
// Menus (shared Create menu for both the main menu bar and the Hierarchy context menu)
// ---------------------------------------------------------------------------------------
void Editor::DrawMainMenuBar(Scene& scene, RenderSettings& settings)
{
	if (!ImGui::BeginMainMenuBar()) return;

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New Scene")) NewScene(scene);
		if (ImGui::MenuItem("Save Scene", "Ctrl+S")) SaveScene(scene);
		if (ImGui::MenuItem("Load Scene", "Ctrl+O")) LoadScene(scene);
		ImGui::Separator();
		ImGui::TextDisabled("Path: %s", scenePath.c_str());
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Create"))
	{
		if (ImGui::MenuItem("Empty GameObject")) CreateEmpty(scene);
		if (ImGui::MenuItem("Camera")) CreateCamera(scene);
		if (ImGui::BeginMenu("Primitive"))
		{
			if (ImGui::MenuItem("Cube"))     CreatePrimitive(scene, PrimitiveType::Cube);
			if (ImGui::MenuItem("Sphere"))   CreatePrimitive(scene, PrimitiveType::Sphere);
			if (ImGui::MenuItem("Plane"))    CreatePrimitive(scene, PrimitiveType::Plane);
			if (ImGui::MenuItem("Cylinder")) CreatePrimitive(scene, PrimitiveType::Cylinder);
			if (ImGui::MenuItem("Capsule"))  CreatePrimitive(scene, PrimitiveType::Capsule);
			if (ImGui::MenuItem("Cone"))     CreatePrimitive(scene, PrimitiveType::Cone);
			if (ImGui::MenuItem("Torus"))    CreatePrimitive(scene, PrimitiveType::Torus);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Light"))
		{
			if (ImGui::MenuItem("Directional Light")) CreateLight(scene, (int)LightComponent::Type::Directional);
			if (ImGui::MenuItem("Point Light"))       CreateLight(scene, (int)LightComponent::Type::Point);
			if (ImGui::MenuItem("Spot Light"))        CreateLight(scene, (int)LightComponent::Type::Spot);
			if (ImGui::MenuItem("Rectangle Light"))   CreateLight(scene, (int)LightComponent::Type::Rect);
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem("Undo", "Ctrl+Z", false, history.CanUndo()))
		{ std::string n = history.Undo(scene); if (!n.empty()) Log("Undo: " + n); }
		if (ImGui::MenuItem("Redo", "Ctrl+Y", false, history.CanRedo()))
		{ std::string n = history.Redo(scene); if (!n.empty()) Log("Redo: " + n); }
		ImGui::Separator();
		if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, scene.SelectedObject() != nullptr)) DuplicateSelected(scene);
		if (ImGui::MenuItem("Delete", "Del", false, scene.SelectedObject() != nullptr)) DeleteSelected(scene);
		bool hasParent = scene.SelectedObject() && scene.SelectedObject()->parentId != 0;
		if (ImGui::MenuItem("Unparent", nullptr, false, hasParent))
			Reparent(scene, scene.SelectedId(), 0);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Gizmo"))
	{
		bool t = gizmoOp == GizmoOp::Translate, r = gizmoOp == GizmoOp::Rotate, s = gizmoOp == GizmoOp::Scale;
		if (ImGui::MenuItem("Translate", "W", t)) gizmoOp = GizmoOp::Translate;
		if (ImGui::MenuItem("Rotate", "E", r))    gizmoOp = GizmoOp::Rotate;
		if (ImGui::MenuItem("Scale", "R", s))     gizmoOp = GizmoOp::Scale;
		ImGui::Separator();
		bool world = gizmoSpace == GizmoSpace::World;
		if (ImGui::MenuItem("World Space", nullptr, world))  gizmoSpace = GizmoSpace::World;
		if (ImGui::MenuItem("Local Space", nullptr, !world)) gizmoSpace = GizmoSpace::Local;
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("View"))
	{
		ImGui::MenuItem("Infinite Grid", nullptr, &settings.showGrid);
		ImGui::MenuItem("Physics Debug", nullptr, &settings.showPhysicsDebug);
		ImGui::DragFloat("Grid Spacing", &settings.gridSpacing, 0.05f, 0.05f, 100.0f);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Window"))
	{
		ImGui::MenuItem("Undo History", nullptr, &showHistoryPanel);
		ImGui::EndMenu();
	}

	// Play / Stop toolbar (right of the menus).
	ImGui::Separator();
	if (!isPlaying)
	{
		if (ImGui::MenuItem("\xE2\x96\xB6 Play")) EnterPlayMode(scene);   // ▶
	}
	else
	{
		if (ImGui::MenuItem("\xE2\x96\xA0 Stop")) ExitPlayMode(scene);    // ■
		ImGui::TextDisabled("  [PLAYING]");
	}

	ImGui::EndMainMenuBar();
}

// ---------------------------------------------------------------------------------------
// Hierarchy
// ---------------------------------------------------------------------------------------
void Editor::Reparent(Scene& scene, std::uint32_t childId, std::uint32_t parentId)
{
	GameObject* c = scene.FindById(childId);
	if (!c) return;
	GameObject before = *c;
	if (scene.SetParent(childId, parentId))
	{
		history.Push(std::make_unique<ModifyCommand>(childId, before, *c, parentId ? "Reparent" : "Unparent"));
		Log(parentId ? "Reparented: " + c->name : "Unparented: " + c->name);
	}
}

void Editor::DrawHierarchyNode(Scene& scene, GameObject* go)
{
	// Does this node have children?
	bool hasChildren = false;
	for (auto& o : scene.Objects())
		if (o->parentId == go->id) { hasChildren = true; break; }

	ImGui::PushID((int)go->id);

	bool active = go->active;
	if (ImGui::Checkbox("##active", &active))
	{
		GameObject before = *go; go->active = active;
		history.Push(std::make_unique<ModifyCommand>(go->id, before, *go, "Toggle Active"));
	}
	ImGui::SameLine();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |
		ImGuiTreeNodeFlags_DefaultOpen;
	if (scene.SelectedId() == go->id) flags |= ImGuiTreeNodeFlags_Selected;
	if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	bool open = ImGui::TreeNodeEx(go->name.c_str(), flags);
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		scene.SelectById(go->id);
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		scene.SelectById(go->id);
		FocusSelected(scene);
	}

	// Drag-drop reparenting: drag this node onto another to parent it there.
	if (ImGui::BeginDragDropSource())
	{
		std::uint32_t id = go->id;
		ImGui::SetDragDropPayload("GAMEOBJECT_ID", &id, sizeof(id));
		ImGui::Text("Move %s", go->name.c_str());
		ImGui::EndDragDropSource();
	}
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("GAMEOBJECT_ID"))
			Reparent(scene, *(const std::uint32_t*)p->Data, go->id);
		ImGui::EndDragDropTarget();
	}

	if (open && hasChildren)
	{
		for (auto& o : scene.Objects())
			if (o->parentId == go->id) DrawHierarchyNode(scene, o.get());
		ImGui::TreePop();
	}

	ImGui::PopID();
}

void Editor::DrawHierarchy(Scene& scene)
{
	ImGui::TextDisabled("Right-click: Create | Drag: reparent");
	ImGui::Separator();

	// Draw roots (parentId == 0); children are drawn recursively.
	for (auto& o : scene.Objects())
		if (o->parentId == 0) DrawHierarchyNode(scene, o.get());

	// A drop target over the remaining empty space detaches an object to the root.
	ImGui::Dummy(ImVec2(0, 24));
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("GAMEOBJECT_ID"))
			Reparent(scene, *(const std::uint32_t*)p->Data, 0);
		ImGui::EndDragDropTarget();
	}

	// Right-click anywhere in the window (empty space included) -> create menu.
	if (ImGui::BeginPopupContextWindow("hierarchy_ctx",
		ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("Empty GameObject")) CreateEmpty(scene);
		if (ImGui::MenuItem("Camera")) CreateCamera(scene);
		if (ImGui::BeginMenu("Primitive"))
		{
			if (ImGui::MenuItem("Cube"))     CreatePrimitive(scene, PrimitiveType::Cube);
			if (ImGui::MenuItem("Sphere"))   CreatePrimitive(scene, PrimitiveType::Sphere);
			if (ImGui::MenuItem("Plane"))    CreatePrimitive(scene, PrimitiveType::Plane);
			if (ImGui::MenuItem("Cylinder")) CreatePrimitive(scene, PrimitiveType::Cylinder);
			if (ImGui::MenuItem("Capsule"))  CreatePrimitive(scene, PrimitiveType::Capsule);
			if (ImGui::MenuItem("Cone"))     CreatePrimitive(scene, PrimitiveType::Cone);
			if (ImGui::MenuItem("Torus"))    CreatePrimitive(scene, PrimitiveType::Torus);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Light"))
		{
			if (ImGui::MenuItem("Directional Light")) CreateLight(scene, (int)LightComponent::Type::Directional);
			if (ImGui::MenuItem("Point Light"))       CreateLight(scene, (int)LightComponent::Type::Point);
			if (ImGui::MenuItem("Spot Light"))        CreateLight(scene, (int)LightComponent::Type::Spot);
			if (ImGui::MenuItem("Rectangle Light"))   CreateLight(scene, (int)LightComponent::Type::Rect);
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}
}

// ---------------------------------------------------------------------------------------
// Inspector
// ---------------------------------------------------------------------------------------
void Editor::TrackEdit(Scene& /*scene*/, GameObject* go, const char* label)
{
	if (ImGui::IsItemActivated())
	{
		editBefore = *go;
		editId = go->id;
		editing = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit() && editing && editId == go->id)
	{
		history.Push(std::make_unique<ModifyCommand>(go->id, editBefore, *go, label));
		editing = false;
	}
}

void Editor::DrawInspector(Scene& scene, RenderSettings& settings)
{
	GameObject* go = scene.SelectedObject();
	if (!go)
	{
		ImGui::TextDisabled("No object selected.");
		ImGui::TextDisabled("Click an object in the viewport or Hierarchy.");
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("Exposure", &settings.exposure, 0.05f, 5.0f);
			ImGui::SliderFloat("Skybox Intensity", &settings.skyboxIntensity, 0.0f, 5.0f);
			ImGui::Checkbox("Use IBL", &settings.useIBL);
			ImGui::Checkbox("Show Skybox", &settings.showSkybox);
			ImGui::Checkbox("Show Grid", &settings.showGrid);
			ImGui::DragFloat("Grid Spacing", &settings.gridSpacing, 0.05f, 0.05f, 100.0f);
			ImGui::Checkbox("Physics Debug", &settings.showPhysicsDebug);
		}
		return;
	}

	// Object header: active, editable name, type.
	{
		bool active = go->active;
		if (ImGui::Checkbox("##goactive", &active))
		{
			GameObject before = *go; go->active = active;
			history.Push(std::make_unique<ModifyCommand>(go->id, before, *go, "Toggle Active"));
		}
		ImGui::SameLine();
		char nameBuf[128];
		std::snprintf(nameBuf, sizeof(nameBuf), "%s", go->name.c_str());
		if (ImGui::InputText("##goname", nameBuf, sizeof(nameBuf)))
			go->name = nameBuf;
		TrackEdit(scene, go, "Rename Object");
		ImGui::TextDisabled("%s  (id %u)", go->TypeLabel(), go->id);
	}
	ImGui::Separator();

	// Transform
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		Transform& t = go->transform;
		ImGui::DragFloat3("Position", &t.position.x, 0.02f); TrackEdit(scene, go, "Move");
		ImGui::DragFloat3("Rotation", &t.rotationEuler.x, 0.5f); TrackEdit(scene, go, "Rotate");
		ImGui::DragFloat3("Scale", &t.scale.x, 0.01f, 0.001f, 1000.0f); TrackEdit(scene, go, "Scale");
		if (ImGui::Button("Reset Transform"))
		{
			GameObject before = *go; t.Reset();
			history.Push(std::make_unique<ModifyCommand>(go->id, before, *go, "Reset Transform"));
		}
	}

	// Mesh Renderer + Material
	if (go->meshRenderer)
	{
		MeshRenderer& mr = *go->meshRenderer;
		if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool en = mr.enabled;
			if (ImGui::Checkbox("Enabled##renderer", &en))
			{
				GameObject before = *go; mr.enabled = en;
				history.Push(std::make_unique<ModifyCommand>(go->id, before, *go, "Toggle Renderer"));
			}
			ImGui::Text("Mesh: %s", mr.meshName.c_str());
			ImGui::Text("Material: %s", mr.materialName.c_str());
		}
		if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
		{
			PbrMaterial& m = mr.material;
			ImGui::TextDisabled("Shader: pbr.vert / pbr.frag");
			ImGui::Separator();
			ImGui::ColorEdit3("Base Color", &m.albedo.x);        TrackEdit(scene, go, "Change Base Color");
			ImGui::SliderFloat("Metallic", &m.metallic, 0.0f, 1.0f);   TrackEdit(scene, go, "Change Metallic");
			ImGui::SliderFloat("Roughness", &m.roughness, 0.0f, 1.0f); TrackEdit(scene, go, "Change Roughness");
			ImGui::SliderFloat("Ambient Occlusion", &m.ao, 0.0f, 1.0f); TrackEdit(scene, go, "Change AO");
			ImGui::Separator();
			ImGui::TextDisabled("Texture Maps");
			DrawTexturePreview("Albedo", m.albedoMap);
			DrawTexturePreview("Normal", m.normalMap);
			DrawTexturePreview("Metallic", m.metallicMap);
			DrawTexturePreview("Roughness", m.roughnessMap);
			DrawTexturePreview("AO", m.aoMap);
		}
	}

	// Light
	if (go->light)
	{
		LightComponent& l = *go->light;
		if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool en = l.enabled;
			if (ImGui::Checkbox("Enabled##light", &en))
			{
				GameObject before = *go; l.enabled = en;
				history.Push(std::make_unique<ModifyCommand>(go->id, before, *go, "Toggle Light"));
			}
			int type = (int)l.type;
			GameObject beforeType = *go;
			if (ImGui::Combo("Type", &type, kLightTypes, IM_ARRAYSIZE(kLightTypes)))
			{
				l.type = (LightComponent::Type)type;
				history.Push(std::make_unique<ModifyCommand>(go->id, beforeType, *go, "Change Light Type"));
			}
			ImGui::ColorEdit3("Color", &l.color.x);                    TrackEdit(scene, go, "Change Light Color");
			ImGui::DragFloat("Intensity", &l.intensity, 0.1f, 0.0f, 10000.0f); TrackEdit(scene, go, "Change Intensity");

			if (l.type == LightComponent::Type::Directional || l.type == LightComponent::Type::Spot ||
				l.type == LightComponent::Type::Rect)
			{
				ImGui::DragFloat3("Direction", &l.direction.x, 0.01f); TrackEdit(scene, go, "Change Direction");
			}
			if (l.type == LightComponent::Type::Spot)
			{
				ImGui::DragFloat("Inner Cutoff", &l.innerCutoffDeg, 0.2f, 0.0f, 89.0f); TrackEdit(scene, go, "Change Inner Cutoff");
				ImGui::DragFloat("Outer Cutoff", &l.outerCutoffDeg, 0.2f, 0.0f, 90.0f); TrackEdit(scene, go, "Change Outer Cutoff");
			}
			if (l.type == LightComponent::Type::Rect)
			{
				ImGui::DragFloat("Width", &l.width, 0.05f, 0.01f, 1000.0f);  TrackEdit(scene, go, "Change Rect Width");
				ImGui::DragFloat("Height", &l.height, 0.05f, 0.01f, 1000.0f); TrackEdit(scene, go, "Change Rect Height");
			}
		}
	}

	// Script
	if (ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (go->script)
		{
			ScriptComponent& sc = *go->script;
			bool en = sc.enabled;
			if (ImGui::Checkbox("Enabled##script", &en))
			{
				GameObject before = *go; sc.enabled = en;
				history.Push(std::make_unique<ModifyCommand>(go->id, before, *go, "Toggle Script"));
			}

			// Behavior picker: "(none)" + registered behaviors.
			std::vector<const char*> items;
			items.push_back("(none)");
			for (const std::string& n : ScriptRegistry::Names()) items.push_back(n.c_str());
			int current = 0;
			for (int i = 1; i < (int)items.size(); ++i)
				if (sc.behaviorName == items[i]) { current = i; break; }
			GameObject beforeSel = *go;
			if (ImGui::Combo("Behavior", &current, items.data(), (int)items.size()))
			{
				sc.behaviorName = (current == 0) ? std::string() : items[current];
				sc.params.clear(); // params belong to the behavior; repopulate below
				history.Push(std::make_unique<ModifyCommand>(go->id, beforeSel, *go, "Set Behavior"));
			}
			if (sc.behaviorName.empty())
			{
				ImGui::TextDisabled("No behavior selected.");
			}
			else
			{
				// Behavior-declared parameters, editable per object (defaults filled in once).
				std::vector<ScriptParam> declared = ScriptRegistry::ParamsOf(sc.behaviorName);
				if (!declared.empty())
				{
					ImGui::SeparatorText("Parameters");
					for (const ScriptParam& p : declared)
					{
						if (sc.params.find(p.name) == sc.params.end())
							sc.params[p.name] = p.defaultValue;
						ImGui::DragFloat(p.name.c_str(), &sc.params[p.name], 0.05f);
						TrackEdit(scene, go, "Change Script Param");
					}
				}
			}

			if (ImGui::Button("Remove Script"))
			{
				GameObject before = *go; go->script.reset();
				history.Push(std::make_unique<ModifyCommand>(go->id, before, *go, "Remove Script"));
			}
		}
		else
		{
			if (ImGui::Button("Add Script Component"))
			{
				GameObject before = *go; go->script = ScriptComponent{};
				history.Push(std::make_unique<ModifyCommand>(go->id, before, *go, "Add Script"));
			}
		}
	}
	// Camera (game camera that renders in Play mode)
	{
		if (go->cameraComp)
		{
			if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
			{
				CameraComponent& cc = *go->cameraComp;
				ImGui::DragFloat("Field of View", &cc.fov, 0.5f, 10.0f, 170.0f); TrackEdit(scene, go, "Camera FOV");
				ImGui::DragFloat("Near Clip", &cc.nearClip, 0.01f, 0.001f, 100.0f); TrackEdit(scene, go, "Near Clip");
				ImGui::DragFloat("Far Clip", &cc.farClip, 1.0f, 1.0f, 10000.0f); TrackEdit(scene, go, "Far Clip");
				bool m = cc.isMain;
				if (ImGui::Checkbox("Main Camera", &m)) { GameObject b = *go; cc.isMain = m; history.Push(std::make_unique<ModifyCommand>(go->id, b, *go, "Main Camera")); }
				ImGui::TextDisabled("Play mode renders through this camera.");
			}
		}
		else if (ImGui::Button("Add Camera"))
		{ GameObject b = *go; go->cameraComp = CameraComponent{}; history.Push(std::make_unique<ModifyCommand>(go->id, b, *go, "Add Camera")); }
	}

	// Rigidbody
	{
		const char* kBodyTypes[] = { "Static", "Dynamic", "Kinematic" };
		if (go->rigidBody)
		{
			if (ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen))
			{
				RigidBodyComponent& rb = *go->rigidBody;
				int bt = (int)rb.type; GameObject beforeBt = *go;
				if (ImGui::Combo("Body Type", &bt, kBodyTypes, IM_ARRAYSIZE(kBodyTypes)))
				{ rb.type = (RigidBodyComponent::BodyType)bt; history.Push(std::make_unique<ModifyCommand>(go->id, beforeBt, *go, "Body Type")); }
				ImGui::DragFloat("Mass", &rb.mass, 0.05f, 0.001f, 10000.0f); TrackEdit(scene, go, "Mass");
				bool g = rb.useGravity; if (ImGui::Checkbox("Use Gravity", &g)) { GameObject b = *go; rb.useGravity = g; history.Push(std::make_unique<ModifyCommand>(go->id, b, *go, "Use Gravity")); }
				ImGui::DragFloat("Linear Damping", &rb.linearDamping, 0.01f, 0.0f, 10.0f); TrackEdit(scene, go, "Linear Damping");
				ImGui::DragFloat("Angular Damping", &rb.angularDamping, 0.01f, 0.0f, 10.0f); TrackEdit(scene, go, "Angular Damping");
				ImGui::DragFloat3("Init Linear Vel", &rb.initialLinearVelocity.x, 0.05f); TrackEdit(scene, go, "Init Lin Vel");
				ImGui::DragFloat3("Init Angular Vel", &rb.initialAngularVelocity.x, 0.05f); TrackEdit(scene, go, "Init Ang Vel");
				GameObject bf = *go; bool ch = false;
				bool px = rb.freezePosX, py = rb.freezePosY, pz = rb.freezePosZ;
				bool rx = rb.freezeRotX, ry = rb.freezeRotY, rz = rb.freezeRotZ;
				ImGui::TextDisabled("Freeze Pos"); ImGui::SameLine();
				ImGui::Checkbox("X##fp", &px); ImGui::SameLine(); ImGui::Checkbox("Y##fp", &py); ImGui::SameLine(); ImGui::Checkbox("Z##fp", &pz);
				ImGui::TextDisabled("Freeze Rot"); ImGui::SameLine();
				ImGui::Checkbox("X##fr", &rx); ImGui::SameLine(); ImGui::Checkbox("Y##fr", &ry); ImGui::SameLine(); ImGui::Checkbox("Z##fr", &rz);
				if (px != rb.freezePosX || py != rb.freezePosY || pz != rb.freezePosZ || rx != rb.freezeRotX || ry != rb.freezeRotY || rz != rb.freezeRotZ)
				{ rb.freezePosX = px; rb.freezePosY = py; rb.freezePosZ = pz; rb.freezeRotX = rx; rb.freezeRotY = ry; rb.freezeRotZ = rz; ch = true; }
				if (ch) history.Push(std::make_unique<ModifyCommand>(go->id, bf, *go, "Constraints"));
				if (ImGui::Button("Remove Rigidbody")) { GameObject b = *go; go->rigidBody.reset(); history.Push(std::make_unique<ModifyCommand>(go->id, b, *go, "Remove Rigidbody")); }
			}
		}
		else if (ImGui::Button("Add Rigidbody"))
		{ GameObject b = *go; go->rigidBody = RigidBodyComponent{}; history.Push(std::make_unique<ModifyCommand>(go->id, b, *go, "Add Rigidbody")); }
	}

	// Collider
	{
		const char* kShapes[] = { "Box", "Sphere", "Capsule", "Plane", "Convex Mesh", "Triangle Mesh" };
		if (go->collider)
		{
			if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ColliderComponent& co = *go->collider;
				int sh = (int)co.shape; GameObject beforeSh = *go;
				if (ImGui::Combo("Shape", &sh, kShapes, IM_ARRAYSIZE(kShapes)))
				{ co.shape = (ColliderComponent::Shape)sh; history.Push(std::make_unique<ModifyCommand>(go->id, beforeSh, *go, "Collider Shape")); }
				ImGui::DragFloat3("Center", &co.center.x, 0.02f); TrackEdit(scene, go, "Collider Center");
				if (co.shape == ColliderComponent::Shape::Box)
				{ ImGui::DragFloat3("Half Extents", &co.halfExtents.x, 0.02f, 0.001f, 1000.0f); TrackEdit(scene, go, "Half Extents"); }
				if (co.shape == ColliderComponent::Shape::Sphere || co.shape == ColliderComponent::Shape::Capsule)
				{ ImGui::DragFloat("Radius", &co.radius, 0.02f, 0.001f, 1000.0f); TrackEdit(scene, go, "Radius"); }
				if (co.shape == ColliderComponent::Shape::Capsule)
				{ ImGui::DragFloat("Half Height", &co.halfHeight, 0.02f, 0.001f, 1000.0f); TrackEdit(scene, go, "Half Height"); }
				bool tr = co.isTrigger; if (ImGui::Checkbox("Is Trigger", &tr)) { GameObject b = *go; co.isTrigger = tr; history.Push(std::make_unique<ModifyCommand>(go->id, b, *go, "Trigger")); }
				ImGui::SeparatorText("Physics Material");
				ImGui::DragFloat("Static Friction", &co.material.staticFriction, 0.01f, 0.0f, 2.0f); TrackEdit(scene, go, "Static Friction");
				ImGui::DragFloat("Dynamic Friction", &co.material.dynamicFriction, 0.01f, 0.0f, 2.0f); TrackEdit(scene, go, "Dynamic Friction");
				ImGui::DragFloat("Restitution", &co.material.restitution, 0.01f, 0.0f, 1.0f); TrackEdit(scene, go, "Restitution");
				if (ImGui::Button("Remove Collider")) { GameObject b = *go; go->collider.reset(); history.Push(std::make_unique<ModifyCommand>(go->id, b, *go, "Remove Collider")); }
			}
		}
		else if (ImGui::Button("Add Collider"))
		{ GameObject b = *go; go->collider = ColliderComponent{}; history.Push(std::make_unique<ModifyCommand>(go->id, b, *go, "Add Collider")); }
	}
}

void Editor::DrawConsole(const RenderSettings& settings)
{
	ImGui::Text("FPS: %.1f", settings.fps);
	ImGui::SameLine(160.0f);
	ImGui::TextDisabled("Undo: %s | Redo: %s", history.CanUndo() ? "available" : "-", history.CanRedo() ? "available" : "-");
	ImGui::Separator();
	ImGui::BeginChild("log");
	for (const std::string& line : console)
		ImGui::TextUnformatted(line.c_str());
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);
	ImGui::EndChild();
}

void Editor::DrawHistoryPanel()
{
	ImGui::SetNextWindowSize(ImVec2(260, 320), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Undo History", &showHistoryPanel))
	{
		ImGui::TextDisabled("Undo stack (oldest first)");
		ImGui::Separator();
		for (const auto& c : history.UndoStack())
			ImGui::BulletText("%s", c->Name().c_str());
		if (history.CanRedo())
		{
			ImGui::Separator();
			ImGui::TextDisabled("Redo stack");
			for (auto it = history.RedoStack().rbegin(); it != history.RedoStack().rend(); ++it)
				ImGui::BulletText("%s", (*it)->Name().c_str());
		}
	}
	ImGui::End();
}

// ---------------------------------------------------------------------------------------
// Viewport picking
// ---------------------------------------------------------------------------------------
void Editor::HandlePicking(Scene& scene, const glm::mat4& view, const glm::mat4& proj)
{
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 mouse = io.MousePos;

	bool inViewport =
		mouse.x >= viewportRect.x && mouse.x <= viewportRect.x + viewportRect.z &&
		mouse.y >= viewportRect.y && mouse.y <= viewportRect.y + viewportRect.w;
	bool overUI = io.WantCaptureMouse;        // over a panel
	bool overGizmo = ImGuizmo::IsOver() || ImGuizmo::IsUsing();
	bool navigating = ImGui::IsMouseDown(ImGuiMouseButton_Right);

	// Track a click that begins in the viewport; select only if it wasn't a drag (so a
	// look-drag or gizmo-drag doesn't trigger selection).
	static bool pressedInViewport = false;
	static ImVec2 pressPos;

	if (inViewport && !overUI && !overGizmo && !navigating && !io.KeyAlt &&
		ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		pressedInViewport = true;
		pressPos = mouse;
	}

	if (pressedInViewport && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		pressedInViewport = false;
		float dx = mouse.x - pressPos.x, dy = mouse.y - pressPos.y;
		if (std::sqrt(dx * dx + dy * dy) < 5.0f && !overGizmo) // a click, not a drag
		{
			Picking::Ray ray = Picking::ScreenPointToRay(mouse.x, mouse.y, fbWidth, fbHeight,
				view, proj, camera ? camera->Position : glm::vec3(0.0f));

			std::uint32_t bestId = 0;
			float bestDist = 1e30f;
			for (auto& up : scene.Objects())
			{
				GameObject* go = up.get();
				if (!go->active || !go->meshRenderer) continue;
				glm::mat4 model = scene.WorldMatrix(go);
				float dist;
				if (Picking::RayObjectAABB(ray, model, go->meshRenderer->localMin, go->meshRenderer->localMax, dist))
				{
					if (dist < bestDist) { bestDist = dist; bestId = go->id; }
				}
			}
			if (bestId != 0) { scene.SelectById(bestId); Log("Selected: " + std::string(scene.FindById(bestId)->name)); }
			else scene.Deselect(); // clicking empty space deselects
		}
	}
}

// ---------------------------------------------------------------------------------------
// Camera navigation: scroll-zoom, Alt+LMB orbit around the selection, middle-mouse pan.
// Complements the RMB flythrough (handled by Camera::Inputs). Unity-style.
// ---------------------------------------------------------------------------------------
void Editor::HandleCameraNavigation(Scene& scene)
{
	if (!camera) return;
	ImGuiIO& io = ImGui::GetIO();

	ImVec2 m = io.MousePos;
	bool inViewport =
		m.x >= viewportRect.x && m.x <= viewportRect.x + viewportRect.z &&
		m.y >= viewportRect.y && m.y <= viewportRect.y + viewportRect.w;
	if (!inViewport || io.WantCaptureMouse || ImGuizmo::IsUsing()) return;
	if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) return; // RMB flythrough owns the camera

	// Pivot: the selected object's world position, else a point in front of the camera.
	glm::vec3 fwd = glm::length(camera->Orientation) > 1e-4f ? glm::normalize(camera->Orientation) : glm::vec3(0, 0, -1);
	glm::vec3 pivot;
	if (GameObject* sel = scene.SelectedObject())
		pivot = scene.WorldPosition(sel);
	else
		pivot = camera->Position + fwd * 10.0f;

	float dist = glm::max(glm::length(camera->Position - pivot), 0.05f);
	glm::vec3 right = glm::normalize(glm::cross(fwd, camera->Up));
	glm::vec3 up = glm::normalize(glm::cross(right, fwd));

	// --- Scroll to zoom (dolly along the view direction; speed scales with distance) ---
	if (io.MouseWheel != 0.0f)
		camera->Position += fwd * io.MouseWheel * (dist * 0.15f + 0.4f);

	glm::vec2 d(io.MouseDelta.x, io.MouseDelta.y);

	// --- Alt + Left drag: orbit around the pivot ---
	if (io.KeyAlt && ImGui::IsMouseDown(ImGuiMouseButton_Left) && (d.x != 0.0f || d.y != 0.0f))
	{
		glm::vec3 offset = camera->Position - pivot;
		offset = glm::rotate(offset, glm::radians(-d.x * 0.3f), camera->Up);          // yaw
		glm::vec3 pitched = glm::rotate(offset, glm::radians(-d.y * 0.3f), right);     // pitch
		// Reject pitch that gets too close to the poles (avoid flipping).
		if (std::abs(glm::dot(glm::normalize(pitched), camera->Up)) < 0.985f)
			offset = pitched;
		camera->Position = pivot + offset;
		camera->Orientation = glm::normalize(pivot - camera->Position);
	}

	// --- Middle drag: pan (screen-space translate; scales with distance) ---
	if (ImGui::IsMouseDown(ImGuiMouseButton_Middle) && (d.x != 0.0f || d.y != 0.0f))
	{
		float panScale = dist * 0.0015f;
		camera->Position += (-right * d.x + up * d.y) * panScale;
	}
}

// ---------------------------------------------------------------------------------------
// Transform gizmo (ImGuizmo)
// ---------------------------------------------------------------------------------------
void Editor::DrawGizmo(Scene& scene, const glm::mat4& view, const glm::mat4& proj)
{
	GameObject* go = scene.SelectedObject();
	if (!go)
	{
		gizmoWasUsing = false;
		return;
	}

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
	ImGuizmo::SetRect(viewportRect.x, viewportRect.y, viewportRect.z, viewportRect.w);

	ImGuizmo::OPERATION op =
		gizmoOp == GizmoOp::Translate ? ImGuizmo::TRANSLATE :
		gizmoOp == GizmoOp::Rotate ? ImGuizmo::ROTATE : ImGuizmo::SCALE;
	ImGuizmo::MODE mode = (gizmoSpace == GizmoSpace::World) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;

	// Manipulate the WORLD matrix, then convert back to a local transform relative to the
	// parent so hierarchy transforms stay correct.
	// Hold Ctrl to snap: translate to the grid spacing, rotate 15 deg, scale 0.1.
	float snapT = gridSpacing > 0.0f ? gridSpacing : 1.0f;
	float snapVals[3] = { snapT, snapT, snapT };
	if (gizmoOp == GizmoOp::Rotate) snapVals[0] = snapVals[1] = snapVals[2] = 15.0f;
	else if (gizmoOp == GizmoOp::Scale) snapVals[0] = snapVals[1] = snapVals[2] = 0.1f;
	const float* snap = ImGui::GetIO().KeyCtrl ? snapVals : nullptr;

	glm::mat4 world = scene.WorldMatrix(go);
	ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), op, mode, glm::value_ptr(world),
		nullptr, snap);

	bool using_ = ImGuizmo::IsUsing();
	if (using_)
	{
		glm::mat4 parentWorld(1.0f);
		if (go->parentId != 0)
			if (GameObject* p = scene.FindById(go->parentId)) parentWorld = scene.WorldMatrix(p);
		glm::mat4 local = glm::inverse(parentWorld) * world;

		float t[3], r[3], s[3];
		ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(local), t, r, s);
		go->transform.position = glm::vec3(t[0], t[1], t[2]);
		go->transform.rotationEuler = glm::vec3(r[0], r[1], r[2]);
		go->transform.scale = glm::vec3(s[0], s[1], s[2]);
	}

	// Commit one undo command per drag (snapshot taken on the last non-using frame).
	if (gizmoWasUsing && !using_ && gizmoBefore.id == go->id)
	{
		const char* label = gizmoOp == GizmoOp::Translate ? "Move (gizmo)" :
			gizmoOp == GizmoOp::Rotate ? "Rotate (gizmo)" : "Scale (gizmo)";
		history.Push(std::make_unique<ModifyCommand>(go->id, gizmoBefore, *go, label));
	}
	if (!using_) gizmoBefore = *go; // keep a fresh pre-drag snapshot
	gizmoWasUsing = using_;
}

// ---------------------------------------------------------------------------------------
// Keyboard shortcuts
// ---------------------------------------------------------------------------------------
void Editor::HandleShortcuts(Scene& scene)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantTextInput) return;                              // don't steal typing
	if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) return;    // navigating (flythrough)
	if (isPlaying) return;                                     // scripts own input during Play

	if (io.KeyCtrl)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) { std::string n = history.Undo(scene); if (!n.empty()) Log("Undo: " + n); }
		if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) { std::string n = history.Redo(scene); if (!n.empty()) Log("Redo: " + n); }
		if (ImGui::IsKeyPressed(ImGuiKey_D, false)) DuplicateSelected(scene);
		if (ImGui::IsKeyPressed(ImGuiKey_S, false)) SaveScene(scene);
		if (ImGui::IsKeyPressed(ImGuiKey_O, false)) LoadScene(scene);
		return; // Ctrl combos handled; don't also fire plain-key tools
	}

	if (ImGui::IsKeyPressed(ImGuiKey_W, false)) gizmoOp = GizmoOp::Translate;
	if (ImGui::IsKeyPressed(ImGuiKey_E, false)) gizmoOp = GizmoOp::Rotate;
	if (ImGui::IsKeyPressed(ImGuiKey_R, false)) gizmoOp = GizmoOp::Scale;
	if (ImGui::IsKeyPressed(ImGuiKey_F, false)) FocusSelected(scene);
	if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) DeleteSelected(scene);
}

// ---------------------------------------------------------------------------------------
// Camera focus
// ---------------------------------------------------------------------------------------
void Editor::FocusSelected(Scene& scene)
{
	GameObject* go = scene.SelectedObject();
	if (!go || !camera) return;

	glm::vec3 center;
	float radius;
	glm::mat4 world = scene.WorldMatrix(go);
	if (go->meshRenderer)
	{
		glm::vec3 mn, mx; WorldBounds(*go, world, mn, mx);
		center = (mn + mx) * 0.5f;
		radius = glm::length(mx - mn) * 0.5f;
	}
	else
	{
		center = glm::vec3(world[3]);
		radius = 1.0f;
	}
	radius = glm::max(radius, 0.1f);

	float fovY = glm::radians(60.0f);
	float dist = (radius / std::tan(fovY * 0.5f)) * 1.4f;
	dist = glm::max(dist, radius + 0.5f);

	// Preserve the current viewing direction; just reposition to frame the object.
	glm::vec3 dir = glm::length(camera->Orientation) > 0.0001f ? glm::normalize(camera->Orientation) : glm::vec3(0, 0, -1);
	camera->Position = center - dir * dist;
	Log("Focused: " + go->name);
}

// ---------------------------------------------------------------------------------------
// Scene operations (undoable)
// ---------------------------------------------------------------------------------------
GameObject* Editor::Commit(Scene& scene, std::unique_ptr<GameObject> go, const char* label)
{
	go->id = 0; // let the Scene assign a fresh stable id
	GameObject* added = scene.Add(std::move(go));
	scene.SelectById(added->id);
	history.Push(std::make_unique<AddCommand>(*added, scene.Count() - 1, label));
	Log(std::string(label) + ": " + added->name);
	return added;
}

static glm::vec3 SpawnPosition(Camera* cam)
{
	if (!cam) return glm::vec3(0.0f);
	glm::vec3 dir = glm::length(cam->Orientation) > 0.0001f ? glm::normalize(cam->Orientation) : glm::vec3(0, 0, -1);
	return cam->Position + dir * 6.0f;
}

void Editor::CreateEmpty(Scene& scene)
{
	auto go = std::make_unique<GameObject>();
	go->name = "GameObject";
	go->transform.position = SpawnPosition(camera);
	Commit(scene, std::move(go), "Create Empty");
}

void Editor::CreatePrimitive(Scene& scene, PrimitiveType type)
{
	if (!primitives) return;
	const PrimitiveDef& def = primitives->Get(type);
	auto go = std::make_unique<GameObject>();
	go->name = def.name;
	go->transform.position = SpawnPosition(camera);
	MeshRenderer mr;
	mr.kind = MeshRenderer::Geometry::Mesh;
	mr.mesh = def.mesh;
	mr.sourceId = def.name; // primitive name, for serialization
	mr.meshName = def.name;
	mr.materialName = "Default";
	mr.material.albedo = glm::vec3(0.8f);
	mr.material.metallic = 0.0f;
	mr.material.roughness = 0.5f;
	mr.material.ao = 1.0f;
	mr.localMin = def.localMin;
	mr.localMax = def.localMax;
	go->meshRenderer = mr;
	Commit(scene, std::move(go), "Create Primitive");
}

void Editor::CreateLight(Scene& scene, int lightType)
{
	auto go = std::make_unique<GameObject>();
	go->transform.position = SpawnPosition(camera);
	LightComponent lc;
	lc.type = (LightComponent::Type)lightType;
	switch (lc.type)
	{
	case LightComponent::Type::Directional:
		go->name = "Directional Light"; lc.intensity = 2.5f;
		lc.direction = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.5f)); break;
	case LightComponent::Type::Point:
		go->name = "Point Light"; lc.intensity = 200.0f; break;
	case LightComponent::Type::Spot:
		go->name = "Spot Light"; lc.intensity = 400.0f;
		lc.direction = glm::vec3(0, -1, 0); break;
	case LightComponent::Type::Rect:
		go->name = "Rectangle Light"; lc.intensity = 20.0f;
		lc.direction = glm::vec3(0, 0, -1); lc.width = 4.0f; lc.height = 2.0f; break;
	}
	go->light = lc;
	Commit(scene, std::move(go), "Create Light");
}

void Editor::CreateCamera(Scene& scene)
{
	auto go = std::make_unique<GameObject>();
	go->name = "Camera";
	// Place it where the editor camera is now, looking the same way, so Play starts framed well.
	if (camera)
	{
		go->transform.position = camera->Position;
		glm::vec3 fwd = glm::length(camera->Orientation) > 1e-4f ? glm::normalize(camera->Orientation) : glm::vec3(0, 0, -1);
		float yaw = glm::degrees(std::atan2(-fwd.x, -fwd.z));
		float pitch = glm::degrees(std::asin(glm::clamp(fwd.y, -1.0f, 1.0f)));
		go->transform.rotationEuler = glm::vec3(pitch, yaw, 0.0f);
	}
	CameraComponent cc;
	cc.isMain = (scene.ActiveCamera() == nullptr); // first camera becomes main
	go->cameraComp = cc;
	Commit(scene, std::move(go), "Create Camera");
}

void Editor::DuplicateSelected(Scene& scene)
{
	GameObject* src = scene.SelectedObject();
	if (!src) return;
	auto go = std::make_unique<GameObject>(*src);
	go->name = src->name + " (Copy)";
	go->transform.position += glm::vec3(1.0f, 0.0f, 0.0f);
	Commit(scene, std::move(go), "Duplicate");
}

void Editor::DeleteSelected(Scene& scene)
{
	GameObject* go = scene.SelectedObject();
	if (!go) return;
	int idx = scene.Selected();
	GameObject copy = *go;
	std::string name = go->name;
	scene.RemoveById(go->id);
	history.Push(std::make_unique<RemoveCommand>(copy, (size_t)idx, "Delete " + name));
	Log("Deleted: " + name);
}

// ---------------------------------------------------------------------------------------
// Scene file I/O
// ---------------------------------------------------------------------------------------
void Editor::NewScene(Scene& scene)
{
	scene.Clear();
	history.Clear();
	Log("New (empty) scene.");
}

void Editor::SaveScene(Scene& scene)
{
	std::string err;
	if (SceneIO::Save(scene, scenePath, &err))
		Log("Saved scene -> " + scenePath + " (" + std::to_string(scene.Count()) + " objects)");
	else
		Log("Save failed: " + err);
}

void Editor::LoadScene(Scene& scene)
{
	if (!assets || !primitives) { Log("Load failed: asset systems unavailable."); return; }
	std::string err;
	if (SceneIO::Load(scene, scenePath, *assets, *primitives, &err))
	{
		history.Clear(); // ids/objects changed; previous commands no longer apply
		Log("Loaded scene <- " + scenePath + " (" + std::to_string(scene.Count()) + " objects)");
	}
	else
		Log("Load failed: " + err);
}

// ---------------------------------------------------------------------------------------
// Play mode
// ---------------------------------------------------------------------------------------
void Editor::EnterPlayMode(Scene& scene)
{
	if (isPlaying || !scripts || !input) return;
	playSnapshot = SceneIO::SaveToString(scene); // capture edit-time state
	if (physics) physics->Begin(scene);
	scripts->Begin(scene, *input, physics, 0.0f);
	isPlaying = true;
	Log(std::string("Play: entered play mode") + (physics && physics->Available() ? " (PhysX active)." : "."));
}

void Editor::ExitPlayMode(Scene& scene)
{
	if (!isPlaying) return;
	if (scripts) scripts->End();
	if (physics) physics->End();
	if (assets && primitives && !playSnapshot.empty())
	{
		std::string err;
		if (!SceneIO::LoadFromString(scene, playSnapshot, *assets, *primitives, &err))
			Log("Stop: restore failed: " + err);
	}
	history.Clear(); // objects were rebuilt from the snapshot
	isPlaying = false;
	Log("Stop: scene restored to pre-play state.");
}

void Editor::SetPlayMode(Scene& scene, bool play)
{
	if (play) EnterPlayMode(scene); else ExitPlayMode(scene);
}

void Editor::Update(Scene& scene, float dt)
{
	if (!isPlaying || !scripts || !input) return;
	scripts->Update(scene, *input, physics, dt);        // scripts set velocities / kinematics
	if (physics) physics->Step(scene, dt);              // simulate + write transforms back
	scripts->DispatchPhysicsEvents(scene, *input, physics, dt); // OnCollision / OnTrigger
}

// ---------------------------------------------------------------------------------------
// Misc
// ---------------------------------------------------------------------------------------
bool Editor::WantCaptureMouse() const { return initialized && ImGui::GetIO().WantCaptureMouse; }
bool Editor::WantCaptureKeyboard() const { return initialized && ImGui::GetIO().WantCaptureKeyboard; }
bool Editor::WantCaptureViewport() const { return initialized && (ImGuizmo::IsUsing() || ImGuizmo::IsOver()); }

void Editor::Log(const std::string& msg)
{
	console.push_back(msg);
	if (console.size() > 200) console.erase(console.begin());
}

} // namespace ogl