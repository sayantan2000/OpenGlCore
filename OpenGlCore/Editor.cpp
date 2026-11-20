#include "Editor.h"
#include "Scene.h"
#include "GameObject.h"
#include "PbrMaterial.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cstring>
#include <cstdio>

// ---------------------------------------------------------------------------------------
// Reusable component drawers (one collapsible section each). They edit live data by ref.
// ---------------------------------------------------------------------------------------
namespace
{
	ImTextureID TexID(GLuint id) { return (ImTextureID)(intptr_t)id; }

	void DrawTexturePreview(const char* label, GLuint tex)
	{
		ImGui::Text("%s", label);
		ImGui::SameLine(160.0f);
		if (tex != 0)
			ImGui::Image(TexID(tex), ImVec2(48, 48));
		else
			ImGui::TextDisabled("(none)");
	}

	void DrawTransformSection(Transform& t)
	{
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat3("Position", &t.position.x, 0.02f);
			ImGui::DragFloat3("Rotation", &t.rotationEuler.x, 0.5f); // degrees
			ImGui::DragFloat3("Scale", &t.scale.x, 0.01f, 0.001f, 1000.0f);
			if (ImGui::Button("Reset Transform"))
				t.Reset();
		}
	}

	void DrawMaterialSection(MeshRenderer& mr)
	{
		if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
		{
			PbrMaterial& m = mr.material;
			ImGui::Text("Material: %s", mr.materialName.c_str());
			ImGui::TextDisabled("Shader: pbr.vert / pbr.frag");
			ImGui::Separator();

			ImGui::ColorEdit3("Base Color", &m.albedo.x);
			ImGui::SliderFloat("Metallic", &m.metallic, 0.0f, 1.0f);
			ImGui::SliderFloat("Roughness", &m.roughness, 0.0f, 1.0f);
			ImGui::SliderFloat("Ambient Occlusion", &m.ao, 0.0f, 1.0f);
			ImGui::Separator();

			ImGui::TextDisabled("Texture Maps");
			DrawTexturePreview("Albedo", m.albedoMap);
			DrawTexturePreview("Normal", m.normalMap);
			DrawTexturePreview("Metallic", m.metallicMap);
			DrawTexturePreview("Roughness", m.roughnessMap);
			DrawTexturePreview("AO", m.aoMap);
		}
	}

	void DrawMeshRendererSection(MeshRenderer& mr)
	{
		if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Enabled##renderer", &mr.enabled);
			ImGui::Text("Mesh: %s", mr.meshName.c_str());
			ImGui::Text("Material: %s", mr.materialName.c_str());
		}
		DrawMaterialSection(mr);
	}

	const char* kLightTypes[] = { "Directional", "Point", "Spot" };

	void DrawLightSection(LightComponent& l)
	{
		if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Enabled##light", &l.enabled);
			int type = (int)l.type;
			if (ImGui::Combo("Type", &type, kLightTypes, IM_ARRAYSIZE(kLightTypes)))
				l.type = (LightComponent::Type)type;

			ImGui::ColorEdit3("Color", &l.color.x);
			ImGui::DragFloat("Intensity", &l.intensity, 0.1f, 0.0f, 10000.0f);

			if (l.type == LightComponent::Type::Directional || l.type == LightComponent::Type::Spot)
				ImGui::DragFloat3("Direction", &l.direction.x, 0.01f);
			if (l.type == LightComponent::Type::Spot)
			{
				ImGui::DragFloat("Inner Cutoff", &l.innerCutoffDeg, 0.2f, 0.0f, 89.0f);
				ImGui::DragFloat("Outer Cutoff", &l.outerCutoffDeg, 0.2f, 0.0f, 90.0f);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------
// Editor
// ---------------------------------------------------------------------------------------
void Editor::Init(GLFWwindow* window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef ImGuiConfigFlags_DockingEnable
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	initialized = true;
	Log("Editor initialized (Dear ImGui).");
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

#ifdef ImGuiConfigFlags_DockingEnable
	// Dockspace over the main viewport with a passthrough central node, so the 3D scene
	// rendered to the default framebuffer shows through the empty center.
	ImGuiViewport* vp = ImGui::GetMainViewport();
	ImGui::DockSpaceOverViewport(vp, ImGuiDockNodeFlags_PassthruCentralNode);
#endif
}

void Editor::BuildUI(Scene& scene, RenderSettings& settings)
{
	DrawHierarchy(scene);
	DrawInspector(scene, settings);
	DrawConsole(settings);
}

void Editor::DrawHierarchy(Scene& scene)
{
	ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(260, 420), ImGuiCond_FirstUseEver);
	ImGui::Begin("Scene Hierarchy");

	for (int i = 0; i < (int)scene.Count(); ++i)
	{
		GameObject* go = scene.At(i);
		ImGui::PushID(i);
		bool active = go->active;
		if (ImGui::Checkbox("##active", &active))
			go->active = active;
		ImGui::SameLine();
		bool selected = (scene.Selected() == i);
		if (ImGui::Selectable(go->name.c_str(), selected))
			scene.Select(i);
		ImGui::PopID();
	}

	ImGui::Separator();
	if (scene.Selected() >= 0)
	{
		if (ImGui::Button("Delete Selected"))
			scene.Remove((size_t)scene.Selected());
	}
	else
	{
		ImGui::TextDisabled("Nothing selected");
	}
	ImGui::End();
}

void Editor::DrawInspector(Scene& scene, RenderSettings& settings)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 350, 30), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(340, 560), ImGuiCond_FirstUseEver);
	ImGui::Begin("Inspector");

	GameObject* go = scene.SelectedObject();
	if (!go)
	{
		ImGui::TextDisabled("No object selected.");
		ImGui::TextDisabled("Select an object in the Scene Hierarchy.");
		ImGui::End();
		return;
	}

	// Object header: active, editable name, type.
	ImGui::Checkbox("##goactive", &go->active);
	ImGui::SameLine();
	char nameBuf[128];
	std::snprintf(nameBuf, sizeof(nameBuf), "%s", go->name.c_str());
	if (ImGui::InputText("##goname", nameBuf, sizeof(nameBuf)))
		go->name = nameBuf;
	ImGui::TextDisabled("%s", go->TypeLabel());
	ImGui::Separator();

	// Component sections.
	DrawTransformSection(go->transform);
	if (go->meshRenderer)
		DrawMeshRendererSection(*go->meshRenderer);
	if (go->light)
		DrawLightSection(*go->light);

	ImGui::Separator();
	if (ImGui::CollapsingHeader("Render Settings"))
	{
		ImGui::SliderFloat("Exposure", &settings.exposure, 0.05f, 5.0f);
		ImGui::SliderFloat("Skybox Intensity", &settings.skyboxIntensity, 0.0f, 5.0f);
		ImGui::Checkbox("Use IBL", &settings.useIBL);
		ImGui::Checkbox("Show Skybox", &settings.showSkybox);
	}

	ImGui::End();
}

void Editor::DrawConsole(const RenderSettings& settings)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 170), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 20, 160), ImGuiCond_FirstUseEver);
	ImGui::Begin("Console");
	ImGui::Text("FPS: %.1f", settings.fps);
	ImGui::Separator();
	ImGui::BeginChild("log");
	for (const std::string& line : console)
		ImGui::TextUnformatted(line.c_str());
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);
	ImGui::EndChild();
	ImGui::End();
}

void Editor::EndFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool Editor::WantCaptureMouse() const
{
	return initialized && ImGui::GetIO().WantCaptureMouse;
}
bool Editor::WantCaptureKeyboard() const
{
	return initialized && ImGui::GetIO().WantCaptureKeyboard;
}

void Editor::Log(const std::string& msg)
{
	console.push_back(msg);
	if (console.size() > 200)
		console.erase(console.begin());
}
