// OpenGlCore Engine Hub — a standalone application (separate process from the editor) for
// managing projects: create, open, recent list, and launching the editor with a project path.
// Built on the same GLFW + Dear ImGui + OpenGL stack as the engine, but independent of the engine
// runtime (no renderer, physics or asset-loading code linked here).

#include <cstdio>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "ProjectRegistry.h"
#include "ProjectTemplate.h"

#include <string>
#include <vector>
#include <cstring>
#include <filesystem>
#include <iostream>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace fs = std::filesystem;

static const char* kEngineVersion = "0.1.0";

// ---- Launch the editor as a separate process with the selected project ----
static bool LaunchEditor(const std::string& projectPath, std::string& err)
{
#if defined(_WIN32)
	wchar_t exePathW[MAX_PATH];
	GetModuleFileNameW(nullptr, exePathW, MAX_PATH);
	fs::path hubDir = fs::path(exePathW).parent_path();
	fs::path editor = hubDir / "OpenGlCore.exe";
	if (!fs::exists(editor)) { err = "editor not found next to the Hub: " + editor.string(); return false; }

	// Build a command line that quotes paths so spaces/special chars are safe.
	std::wstring editorW = editor.wstring();
	std::wstring projW(projectPath.begin(), projectPath.end());
	std::wstring cmd = L"\"" + editorW + L"\" --project \"" + projW + L"\"";

	std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
	cmdBuf.push_back(L'\0');

	STARTUPINFOW si{}; si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};
	std::wstring cwd = hubDir.wstring(); // editor's assets (Shader/, Images/) sit next to its exe
	BOOL ok = CreateProcessW(editorW.c_str(), cmdBuf.data(), nullptr, nullptr, FALSE,
		0, nullptr, cwd.c_str(), &si, &pi);
	if (!ok) { err = "CreateProcess failed (code " + std::to_string(GetLastError()) + ")"; return false; }
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return true;
#else
	std::string cmd = "./OpenGlCore --project \"" + projectPath + "\" &";
	return std::system(cmd.c_str()) == 0;
#endif
}

// Headless validation of the Hub's non-GUI logic (project scaffolding + registry), so it can be
// verified in an environment with no display. Run: EngineHub.exe --selftest
static int RunSelfTest()
{
	namespace fs = std::filesystem;
	std::error_code ec;
	std::string loc = (fs::temp_directory_path(ec) / "OpenGlCoreHubTest").string();
	fs::remove_all(loc, ec);

	std::string projPath, err;
	bool created = hub::CreateProject(loc, "DemoProj", hub::Template::Starter, kEngineVersion, &projPath, &err);
	bool valid = created && hub::IsValidProject(projPath);
	bool hasScene = fs::exists(fs::path(projPath) / "Assets" / "Scenes" / "Main.scene", ec);
	bool hasSettings = fs::exists(fs::path(projPath) / "ProjectSettings" / "project.json", ec);
	std::string name = valid ? hub::ReadProjectName(projPath) : std::string();

	// Reject creating over a non-empty folder.
	std::string e2; bool rejectDup = !hub::CreateProject(loc, "DemoProj", hub::Template::Blank, kEngineVersion, &projPath, &e2);

	// Registry round-trip (use an isolated file by redirecting via the real path is fine here).
	hub::ProjectRegistry reg; reg.Load();
	size_t before = reg.Entries().size();
	reg.Touch(projPath, name);
	bool added = !reg.Entries().empty() && reg.Entries().front().path == projPath;
	reg.Remove(projPath);
	bool removed = reg.Entries().size() == before;

	fs::remove_all(loc, ec);

	bool pass = created && valid && hasScene && hasSettings && (name == "DemoProj") && rejectDup && added && removed;
	std::cout << "[HUB-SELFTEST] created=" << created << " valid=" << valid << " scene=" << hasScene
		<< " settings=" << hasSettings << " name='" << name << "' rejectDup=" << rejectDup
		<< " regAdd=" << added << " regRemove=" << removed << "\n";
	std::cout << "[HUB-SELFTEST] " << (pass ? "PASS" : "FAIL") << "\n";
	return pass ? 0 : 1;
}

int main(int argc, char** argv)
{
	if (argc > 1 && std::string(argv[1]) == "--selftest") return RunSelfTest();

	if (!glfwInit()) { std::cerr << "[Hub] glfwInit failed\n"; return -1; }
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(900, 600, "OpenGlCore Hub", nullptr, nullptr);
	if (!window) { std::cerr << "[Hub] window creation failed\n"; glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().IniFilename = nullptr;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	hub::ProjectRegistry registry;
	registry.Load();

	// UI state.
	char newName[128] = "MyGame";
	char newLocation[512] = "";
	char openPath[512] = "";
	int templateIdx = 1; // 0 blank, 1 starter
	std::string status;

	// Default new-project location = <user profile>/OpenGlCoreProjects.
	{
#if defined(_WIN32)
		char* up = nullptr; size_t n = 0;
		if (_dupenv_s(&up, &n, "USERPROFILE") == 0 && up)
		{ std::string d = std::string(up) + "\\OpenGlCoreProjects"; std::snprintf(newLocation, sizeof(newLocation), "%s", d.c_str()); free(up); }
#endif
	}

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		const ImGuiViewport* vp = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(vp->WorkPos);
		ImGui::SetNextWindowSize(vp->WorkSize);
		ImGui::Begin("OpenGlCore Hub", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImGui::Text("OpenGlCore Hub");
		ImGui::SameLine(); ImGui::TextDisabled("engine %s", kEngineVersion);
		ImGui::Separator();

		if (ImGui::BeginTabBar("##hubtabs"))
		{
			// ---- Projects (recent) ----
			if (ImGui::BeginTabItem("Projects"))
			{
				ImGui::TextDisabled("Recent projects (most recent first)");
				ImGui::BeginChild("recent", ImVec2(0, 380), true);
				std::string toOpen, toRemove;
				if (registry.Entries().empty())
					ImGui::TextDisabled("No recent projects. Create or open one from the other tabs.");
				for (const hub::ProjectEntry& e : registry.Entries())
				{
					ImGui::PushID(e.path.c_str());
					bool valid = hub::IsValidProject(e.path);
					ImGui::Text("%s", e.name.c_str());
					ImGui::SameLine(220); ImGui::TextDisabled("%s", e.path.c_str());
					ImGui::SameLine(0, 10);
					if (!valid) { ImGui::SameLine(); ImGui::TextColored(ImVec4(1,0.5f,0.3f,1), "[missing]"); }
					if (ImGui::SmallButton("Open") && valid) toOpen = e.path;
					ImGui::SameLine();
					if (ImGui::SmallButton("Remove")) toRemove = e.path;
					ImGui::Separator();
					ImGui::PopID();
				}
				ImGui::EndChild();
				if (!toRemove.empty()) registry.Remove(toRemove);
				if (!toOpen.empty())
				{
					std::string err;
					if (LaunchEditor(toOpen, err)) { registry.Touch(toOpen, hub::ReadProjectName(toOpen)); status = "Launched editor: " + toOpen; }
					else status = "Launch failed: " + err;
				}
				ImGui::EndTabItem();
			}

			// ---- Create project ----
			if (ImGui::BeginTabItem("New Project"))
			{
				ImGui::InputText("Name", newName, sizeof(newName));
				ImGui::InputText("Location", newLocation, sizeof(newLocation));
				const char* templates[] = { "Blank 3D", "Starter 3D" };
				ImGui::Combo("Template", &templateIdx, templates, 2);
				ImGui::TextDisabled("Creates <Location>/<Name> with the standard project structure.");
				if (ImGui::Button("Create & Open", ImVec2(160, 0)))
				{
					std::string projPath, err;
					hub::Template tmpl = (templateIdx == 1) ? hub::Template::Starter : hub::Template::Blank;
					if (hub::CreateProject(newLocation, newName, tmpl, kEngineVersion, &projPath, &err))
					{
						registry.Touch(projPath, newName);
						std::string lerr;
						if (LaunchEditor(projPath, lerr)) status = "Created and launched: " + projPath;
						else status = "Created, but launch failed: " + lerr;
					}
					else status = "Create failed: " + err;
				}
				ImGui::EndTabItem();
			}

			// ---- Open existing ----
			if (ImGui::BeginTabItem("Open Project"))
			{
				ImGui::InputText("Project folder", openPath, sizeof(openPath));
				ImGui::TextDisabled("Folder containing Assets/ and ProjectSettings/project.json.");
				if (ImGui::Button("Open", ImVec2(120, 0)))
				{
					if (!hub::IsValidProject(openPath)) status = "Not a valid project: " + std::string(openPath);
					else
					{
						std::string err;
						if (LaunchEditor(openPath, err)) { registry.Touch(openPath, hub::ReadProjectName(openPath)); status = "Launched: " + std::string(openPath); }
						else status = "Launch failed: " + err;
					}
				}
				ImGui::EndTabItem();
			}

			// ---- About ----
			if (ImGui::BeginTabItem("About"))
			{
				ImGui::TextWrapped("OpenGlCore Hub manages engine projects and launches the editor.");
				ImGui::Text("Engine version: %s", kEngineVersion);
				ImGui::Text("Registry: %s", hub::ProjectRegistry::RegistryFilePath().c_str());
				ImGui::Separator();
				ImGui::TextDisabled("The Hub is a separate process; it passes the project path to the");
				ImGui::TextDisabled("editor via: OpenGlCore --project \"<path>\".");
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::Separator();
		if (!status.empty()) ImGui::TextWrapped("%s", status.c_str());
		ImGui::End();

		int w, h; glfwGetFramebufferSize(window, &w, &h);
		glViewport(0, 0, w, h);
		glClearColor(0.11f, 0.12f, 0.14f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
