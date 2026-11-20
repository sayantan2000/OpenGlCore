#include "Editor.h"
#include "Scene.h"
#include "GameObject.h"
#include "PbrMaterial.h"
#include "Camera.h"
#include "Picking.h"

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

	// World-space AABB of a mesh object's local bounds under its transform (8 corners).
	void WorldBounds(const GameObject& go, glm::vec3& outMin, glm::vec3& outMax)
	{
		glm::mat4 m = go.transform.Matrix();
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
void Editor::Init(GLFWwindow* window, Camera* cam, const PrimitiveLibrary* prims)
{
	camera = cam;
	primitives = prims;

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
	Log("Hold RIGHT mouse to fly (WASD). Left-click to select. W/E/R = move/rotate/scale, F = focus.");
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

	DrawMainMenuBar(scene);

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
	HandleShortcuts(scene);
}

// ---------------------------------------------------------------------------------------
// Menus (shared Create menu for both the main menu bar and the Hierarchy context menu)
// ---------------------------------------------------------------------------------------
void Editor::DrawMainMenuBar(Scene& scene)
{
	if (!ImGui::BeginMainMenuBar()) return;

	if (ImGui::BeginMenu("Create"))
	{
		if (ImGui::MenuItem("Empty GameObject")) CreateEmpty(scene);
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

	if (ImGui::BeginMenu("Window"))
	{
		ImGui::MenuItem("Undo History", nullptr, &showHistoryPanel);
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

// ---------------------------------------------------------------------------------------
// Hierarchy
// ---------------------------------------------------------------------------------------
void Editor::DrawHierarchy(Scene& scene)
{
	ImGui::TextDisabled("Right-click for Create menu");
	ImGui::Separator();

	for (int i = 0; i < (int)scene.Count(); ++i)
	{
		GameObject* go = scene.At(i);
		ImGui::PushID((int)go->id);
		bool active = go->active;
		if (ImGui::Checkbox("##active", &active))
		{
			GameObject before = *go; go->active = active;
			history.Push(std::make_unique<ModifyCommand>(go->id, before, *go, "Toggle Active"));
		}
		ImGui::SameLine();
		bool selected = (scene.Selected() == i);
		if (ImGui::Selectable(go->name.c_str(), selected))
			scene.Select(i);
		// Double-click in the hierarchy focuses the object.
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			scene.Select(i);
			FocusSelected(scene);
		}
		ImGui::PopID();
	}

	// Right-click anywhere in the window (empty space included) -> create menu.
	if (ImGui::BeginPopupContextWindow("hierarchy_ctx",
		ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("Empty GameObject")) CreateEmpty(scene);
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

	if (inViewport && !overUI && !overGizmo && !navigating &&
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
				glm::mat4 model = go->transform.Matrix();
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

	glm::mat4 model = go->transform.Matrix();
	ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), op, mode, glm::value_ptr(model));

	bool using_ = ImGuizmo::IsUsing();
	if (using_)
	{
		float t[3], r[3], s[3];
		ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), t, r, s);
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

	if (io.KeyCtrl)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) { std::string n = history.Undo(scene); if (!n.empty()) Log("Undo: " + n); }
		if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) { std::string n = history.Redo(scene); if (!n.empty()) Log("Redo: " + n); }
		if (ImGui::IsKeyPressed(ImGuiKey_D, false)) DuplicateSelected(scene);
		if (ImGui::IsKeyPressed(ImGuiKey_S, false)) Log("Ctrl+S: scene serialization is not implemented yet.");
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
	if (go->meshRenderer)
	{
		glm::vec3 mn, mx; WorldBounds(*go, mn, mx);
		center = (mn + mx) * 0.5f;
		radius = glm::length(mx - mn) * 0.5f;
	}
	else
	{
		center = go->transform.position;
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
