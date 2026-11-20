#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <string>
#include "Window.h"
#include "Camera.h"
#include "Mesh.h"
#include "Model.h"
#include "Text.h"
#include "IBL.h"
#include "Light.h"
#include "Texture.h"
#include "PbrMaterial.h"
#include "Scene.h"
#include "Editor.h"
#include "PrimitiveLibrary.h"
#include "LineRenderer.h"
#include "AssetManager.h"
#include "Input.h"
#include "ScriptSystem.h"
#include "shaderClass.h"
#include <vector>

// Owns the window, render targets, the Scene, the ImGui Editor, and the main loop. Wires the
// PBR + IBL + HDR pipeline and the editor together; main() just constructs and runs.
class Application
{
public:
	Application();
	~Application();
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void Run();

private:
	void BuildScene();
	void BuildPrimitiveLibrary();     // create shared primitive meshes for the editor
	void CreateHdrTarget(int w, int h);
	void DestroyHdrTarget();
	void ResizeIfNeeded();            // recreate size-dependent targets on framebuffer resize
	void RenderSceneHDR();     // iterate the Scene into the HDR framebuffer + skybox
	void RenderLightGizmos();  // wireframe overlays for lights (incl. rectangle area lights)
	void TonemapToScreen();    // HDR -> default framebuffer (ACES + gamma)
	LightSet GatherLights();   // build a LightSet from the scene's light objects

	Window window;             // first member: constructs the GL context

	std::unique_ptr<Camera> camera;
	std::unique_ptr<Shader> pbrShader;
	std::unique_ptr<Shader> hdrShader;
	std::unique_ptr<IBL>    ibl;
	std::unique_ptr<Text>   hud;

	// Shared texture/model assets loaded by path (owned here; referenced by materials/meshes).
	std::unique_ptr<AssetManager> assets;

	// Shared primitive meshes (cube/sphere/plane/...) referenced by editor-created objects.
	std::vector<std::unique_ptr<Mesh>> primitiveMeshes;
	PrimitiveLibrary primitives;
	std::unique_ptr<LineRenderer> lines;

	Input input;               // keyboard/mouse abstraction (polled each frame)
	ScriptSystem scripts;      // runs script behaviors during Play mode

	std::unique_ptr<Scene> scene;
	Editor editor;
	RenderSettings settings;

	GLuint hdrFBO = 0, hdrColor = 0, hdrDepth = 0;
	GLuint emptyVAO = 0;       // for the fullscreen tonemap triangle
	int fbWidth = 0, fbHeight = 0;

	std::string hudText = "FPS: --";
	float fov = 60.0f;
};

#endif
