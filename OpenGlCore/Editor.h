#ifndef EDITOR_H
#define EDITOR_H

#include <string>
#include <vector>
#include <glm/glm.hpp>

struct GLFWwindow;
class Scene;

// Per-frame render settings the Inspector/menus can tweak.
struct RenderSettings
{
	float exposure = 1.0f;
	float skyboxIntensity = 1.0f;
	bool  useIBL = true;
	bool  showSkybox = true;
	float fps = 0.0f;
};

// Dear ImGui editor: a Unity/Unreal-style dockable layout with a Scene Hierarchy (left),
// a passthrough central viewport (the 3D render shows through), a unified component-based
// Inspector (right), and a Console (bottom). Edits operate directly on the live Scene.
class Editor
{
public:
	void Init(GLFWwindow* window);
	void Shutdown();

	void BeginFrame();                                   // ImGui new frame + dockspace
	void BuildUI(Scene& scene, RenderSettings& settings); // panels (edit live data)
	void EndFrame();                                     // render draw data

	// True while ImGui is using the mouse/keyboard, so the camera should ignore input.
	bool WantCaptureMouse() const;
	bool WantCaptureKeyboard() const;

	void Log(const std::string& msg);

private:
	bool initialized = false;
	std::vector<std::string> console;

	void DrawHierarchy(Scene& scene);
	void DrawInspector(Scene& scene, RenderSettings& settings);
	void DrawConsole(const RenderSettings& settings);
};

#endif
