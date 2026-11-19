#include "Application.h"
#include "Geometry.h"
#include "GLUtils.h"
#include "Time.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <vector>

// Windowed (false) is handy for development; fullscreen (true) matches the prior behavior.
static constexpr bool FULLSCREEN = true;

Application::Application()
	: window(1600, 900, "OpenGLCore - PBR", FULLSCREEN)
{
	fbWidth = window.Width();
	fbHeight = window.Height();

	glEnable(GL_DEPTH_TEST);

	camera = std::make_unique<Camera>(fbWidth, fbHeight, glm::vec3(0.0f, 0.0f, 20.0f));

	pbrShader = std::make_unique<Shader>("./Shader/pbr.vert", "./Shader/pbr.frag");
	hdrShader = std::make_unique<Shader>("./Shader/hdr.vert", "./Shader/hdr.frag");

	// Build IBL (irradiance, prefilter, BRDF LUT) from the procedural sky.
	ibl = std::make_unique<IBL>();

	// Sphere mesh (shared by the material grid).
	{
		std::vector<GLfloat> verts;
		std::vector<GLuint> idx;
		Geometry::BuildSphere(verts, idx, 64, 1.0f);
		sphere = std::make_unique<Mesh>(verts, idx,
			std::vector<VertexAttribute>{ {3, GL_FLOAT}, {3, GL_FLOAT}, {2, GL_FLOAT}, {3, GL_FLOAT} });
	}

	// Loaded model (reuses the PBR shader via DrawPBR).
	model = std::make_unique<Model>("./Models/Cube.fbx");
	modelScale = model->ScaleToFit(3.0f);
	modelCenter = model->Center();

	hud = std::make_unique<Text>("./Fonts/arial.ttf", 48, fbWidth, fbHeight);

	// Lights: 1 directional + 4 point lights (grid corners) + 1 spotlight from above.
	lights.hasDirectional = true;
	lights.directional.direction = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.5f));
	lights.directional.color = glm::vec3(1.0f, 0.97f, 0.9f);
	lights.directional.intensity = 2.5f;

	lights.points.push_back({ glm::vec3(-12.0f,  12.0f, 12.0f), glm::vec3(1.0f, 0.3f, 0.2f), 400.0f });
	lights.points.push_back({ glm::vec3( 12.0f,  12.0f, 12.0f), glm::vec3(0.2f, 0.4f, 1.0f), 400.0f });
	lights.points.push_back({ glm::vec3(-12.0f, -12.0f, 12.0f), glm::vec3(0.3f, 1.0f, 0.4f), 400.0f });
	lights.points.push_back({ glm::vec3( 12.0f, -12.0f, 12.0f), glm::vec3(1.0f, 0.9f, 0.5f), 400.0f });

	SpotLight spot;
	spot.position = glm::vec3(0.0f, 0.0f, 18.0f);
	spot.direction = glm::vec3(0.0f, 0.0f, -1.0f);
	spot.color = glm::vec3(1.0f);
	spot.intensity = 600.0f;
	spot.innerCutoffDeg = 15.0f;
	spot.outerCutoffDeg = 25.0f;
	lights.spots.push_back(spot);

	CreateHdrTarget(fbWidth, fbHeight);
	glGenVertexArrays(1, &emptyVAO);

	// Static PBR sampler bindings (IBL units 0..2).
	pbrShader->Activate();
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_irradianceMap"), 0);
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_prefilterMap"), 1);
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_brdfLUT"), 2);

	glCheckError();
}

void Application::CreateHdrTarget(int w, int h)
{
	glGenFramebuffers(1, &hdrFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

	glGenTextures(1, &hdrColor);
	glBindTexture(GL_TEXTURE_2D, hdrColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColor, 0);

	glGenRenderbuffers(1, &hdrDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, hdrDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, hdrDepth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("HDR framebuffer is not complete");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::RenderSceneHDR()
{
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	glViewport(0, 0, fbWidth, fbHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	pbrShader->Activate();
	pbrShader->SetUniformValueVec3("u_camPos", camera->Position);
	camera->Matrix(*pbrShader, "camMatrix");
	lights.Upload(*pbrShader);

	// IBL inputs.
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_useIBL"), 1);
	glUniform1f(glGetUniformLocation(pbrShader->ID, "u_maxReflectionLod"), ibl->MaxReflectionLod());
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_CUBE_MAP, ibl->IrradianceMap());
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_CUBE_MAP, ibl->PrefilterMap());
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, ibl->BrdfLUT());

	// Material grid: 7x7 spheres, rows vary metallic, columns vary roughness.
	const int N = 7;
	const float spacing = 2.6f;
	// Scalar material (no maps) for the grid.
	GLuint id = pbrShader->ID;
	glUniform1i(glGetUniformLocation(id, "u_hasAlbedoMap"), 0);
	glUniform1i(glGetUniformLocation(id, "u_hasNormalMap"), 0);
	glUniform1i(glGetUniformLocation(id, "u_hasMetallicMap"), 0);
	glUniform1i(glGetUniformLocation(id, "u_hasRoughnessMap"), 0);
	glUniform1i(glGetUniformLocation(id, "u_hasAoMap"), 0);
	glUniform3f(glGetUniformLocation(id, "u_albedo"), 0.9f, 0.1f, 0.1f);
	glUniform1f(glGetUniformLocation(id, "u_ao"), 1.0f);

	for (int row = 0; row < N; ++row)
	{
		float metallic = (float)row / (float)(N - 1);
		glUniform1f(glGetUniformLocation(id, "u_metallic"), metallic);
		for (int col = 0; col < N; ++col)
		{
			float roughness = glm::clamp((float)col / (float)(N - 1), 0.05f, 1.0f);
			glUniform1f(glGetUniformLocation(id, "u_roughness"), roughness);

			glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(
				(col - (N - 1) / 2.0f) * spacing,
				(row - (N - 1) / 2.0f) * spacing,
				0.0f));
			pbrShader->SetUniformMat4("model", m);
			sphere->Draw();
		}
	}

	// Loaded model, placed in front of the grid.
	glm::mat4 mm = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 8.0f));
	mm = glm::scale(mm, glm::vec3(modelScale));
	mm = glm::translate(mm, -modelCenter);
	pbrShader->SetUniformMat4("model", mm);
	model->DrawPBR(*pbrShader);

	// Skybox (environment) last, filling only un-written depth.
	ibl->RenderBackground(
		camera->GetProjectionMatrix(fov, 0.1f, 200.0f),
		camera->GetViewMatrix());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::TonemapToScreen()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fbWidth, fbHeight);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);

	hdrShader->Activate();
	glUniform1i(glGetUniformLocation(hdrShader->ID, "u_hdr"), 0);
	glUniform1f(glGetUniformLocation(hdrShader->ID, "u_exposure"), exposure);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrColor);

	glBindVertexArray(emptyVAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);
}

void Application::Run()
{
	double lastFrame = glfwGetTime();
	double prevFpsTime = lastFrame;
	int frameCount = 0;

	while (!window.ShouldClose())
	{
		double now = glfwGetTime();
		Time::_DeltaTime = (float)(now - lastFrame);
		lastFrame = now;
		frameCount++;

		if (now - prevFpsTime >= 0.5)
		{
			int fps = (int)(frameCount / (now - prevFpsTime));
			double ms = (now - prevFpsTime) / frameCount * 1000.0;
			std::string title = "OpenGLCore - PBR - " + std::to_string(fps) + " FPS / "
				+ std::to_string(ms) + " ms";
			window.SetTitle(title.c_str());
			hudText = "FPS: " + std::to_string(fps);
			prevFpsTime = now;
			frameCount = 0;
		}

		camera->Inputs(window.Handle(), Time::_DeltaTime);
		camera->updateMatrix(fov, 0.1f, 200.0f);

		RenderSceneHDR();
		TonemapToScreen();

		hud->RenderText(hudText, 25.0f, (float)fbHeight - 60.0f, 0.6f, glm::vec3(0.9f, 0.9f, 0.2f));

		window.SwapBuffers();
		window.PollEvents();
	}
}

Application::~Application()
{
	if (hdrColor) glDeleteTextures(1, &hdrColor);
	if (hdrDepth) glDeleteRenderbuffers(1, &hdrDepth);
	if (hdrFBO) glDeleteFramebuffers(1, &hdrFBO);
	if (emptyVAO) glDeleteVertexArrays(1, &emptyVAO);
}
