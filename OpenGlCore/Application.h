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
	std::unique_ptr<Model>  model;
	std::unique_ptr<Text>   hud;

	LightSet lights;
	float modelScale = 1.0f;
	glm::vec3 modelCenter = glm::vec3(0.0f);

	GLuint hdrFBO = 0, hdrColor = 0, hdrDepth = 0;
	GLuint emptyVAO = 0;       // for the fullscreen tonemap triangle
	int fbWidth = 0, fbHeight = 0;

	std::string hudText = "FPS: --";
	float fov = 60.0f;
	float exposure = 1.0f;
};

#endif
