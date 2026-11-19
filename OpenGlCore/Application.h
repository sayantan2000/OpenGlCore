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
#include "shaderClass.h"

// Owns the window, render targets and scene, and runs the main loop. This is the single
// place that wires the PBR + IBL + HDR pipeline together; main() just constructs and runs.
class Application
{
public:
	Application();
	~Application();
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void Run();

private:
	void CreateHdrTarget(int w, int h);
	void RenderSceneHDR();     // geometry + skybox into the HDR framebuffer
	void TonemapToScreen();    // HDR -> default framebuffer (ACES + gamma)

	Window window;             // first member: constructs the GL context

	std::unique_ptr<Camera> camera;
	std::unique_ptr<Shader> pbrShader;
	std::unique_ptr<Shader> hdrShader;
	std::unique_ptr<IBL>    ibl;
	std::unique_ptr<Mesh>   sphere;
	std::unique_ptr<Model>  chair;
	std::unique_ptr<Text>   hud;

	// Chair PBR texture maps (kept alive so their GL ids stay valid) + material.
	std::unique_ptr<Texture> chairAlbedo, chairNormal, chairRoughness, chairAO;
	PbrMaterial chairMaterial;
	float chairScale = 1.0f;
	glm::vec3 chairCenter = glm::vec3(0.0f);

	LightSet lights;

	GLuint hdrFBO = 0, hdrColor = 0, hdrDepth = 0;
	GLuint emptyVAO = 0;       // for the fullscreen tonemap triangle
	int fbWidth = 0, fbHeight = 0;

	std::string hudText = "FPS: --";
	float fov = 60.0f;
	float exposure = 1.0f;
};

#endif
