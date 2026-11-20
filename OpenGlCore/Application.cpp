#include "Application.h"
#include "Geometry.h"
#include "GLUtils.h"
#include "Time.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <vector>

static constexpr bool FULLSCREEN = true;

Application::Application()
	: window(1600, 900, "OpenGLCore - PBR Editor", FULLSCREEN)
{
	fbWidth = window.Width();
	fbHeight = window.Height();

	glEnable(GL_DEPTH_TEST);

	camera = std::make_unique<Camera>(fbWidth, fbHeight, glm::vec3(0.0f, 0.0f, 10.0f));
	pbrShader = std::make_unique<Shader>("./Shader/pbr.vert", "./Shader/pbr.frag");
	hdrShader = std::make_unique<Shader>("./Shader/hdr.vert", "./Shader/hdr.frag");
	ibl = std::make_unique<IBL>("./SKYBOX/ballroom_4k.exr");
	hud = std::make_unique<Text>("./Fonts/arial.ttf", 48, fbWidth, fbHeight);

	// Shared sphere mesh.
	{
		std::vector<GLfloat> verts; std::vector<GLuint> idx;
		Geometry::BuildSphere(verts, idx, 64, 1.0f);
		sphere = std::make_unique<Mesh>(verts, idx,
			std::vector<VertexAttribute>{ {3, GL_FLOAT}, {3, GL_FLOAT}, {2, GL_FLOAT}, {3, GL_FLOAT} });
	}

	// Chair model + its PBR texture set.
	{
		const std::string dir = "./Models/wooden_chair_ukipagtbw_high/";
		const std::string base = dir + "Wooden_Chair_ukipagtbw_High_4K_";
		chair = std::make_unique<Model>((dir + "Wooden_Chair_ukipagtbw_High.fbx").c_str());
		chairAlbedo    = std::make_unique<Texture>((base + "BaseColor.jpg").c_str(), GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
		chairNormal    = std::make_unique<Texture>((base + "Normal.jpg").c_str(),    GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
		chairRoughness = std::make_unique<Texture>((base + "Roughness.jpg").c_str(), GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
		chairAO        = std::make_unique<Texture>((base + "AO.jpg").c_str(),        GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
	}

	CreateHdrTarget(fbWidth, fbHeight);
	glGenVertexArrays(1, &emptyVAO);

	pbrShader->Activate();
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_irradianceMap"), 0);
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_prefilterMap"), 1);
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_brdfLUT"), 2);

	scene = std::make_unique<Scene>();
	BuildScene();

	editor.Init(window.Handle());
	editor.Log(std::string("IBL source: ") + (ibl->UsingHDRI() ? "HDRI (ballroom_4k.exr)" : "procedural sky"));
	editor.Log("Scene loaded: " + std::to_string(scene->Count()) + " objects.");

	glCheckError();
}

void Application::BuildScene()
{
	// --- Wooden chair (Model + full PBR map set) ---
	{
		float fit = chair->ScaleToFit(6.0f);
		glm::vec3 center = chair->Center();
		auto go = std::make_unique<GameObject>();
		go->name = "Wooden Chair";
		go->transform.scale = glm::vec3(fit);
		go->transform.position = -fit * center; // reproduce the centered fit as a TRS
		MeshRenderer mr;
		mr.kind = MeshRenderer::Geometry::Model;
		mr.model = chair.get();
		mr.meshName = "Wooden_Chair_ukipagtbw_High.fbx";
		mr.materialName = "Wood (PBR maps)";
		mr.material.albedo = glm::vec3(1.0f);
		mr.material.metallic = 0.0f;
		mr.material.roughness = 1.0f;
		mr.material.ao = 1.0f;
		mr.material.albedoMap = chairAlbedo->ID;
		mr.material.normalMap = chairNormal->ID;
		mr.material.roughnessMap = chairRoughness->ID;
		mr.material.aoMap = chairAO->ID;
		go->meshRenderer = mr;
		scene->Add(std::move(go));
	}

	// --- Two spheres with scalar materials (metal + dielectric) ---
	auto makeSphere = [&](const char* name, glm::vec3 pos, glm::vec3 color, float metal, float rough)
	{
		auto go = std::make_unique<GameObject>();
		go->name = name;
		go->transform.position = pos;
		go->transform.scale = glm::vec3(1.5f);
		MeshRenderer mr;
		mr.kind = MeshRenderer::Geometry::Mesh;
		mr.mesh = sphere.get();
		mr.meshName = "UV Sphere";
		mr.materialName = name;
		mr.material.albedo = color;
		mr.material.metallic = metal;
		mr.material.roughness = rough;
		mr.material.ao = 1.0f;
		go->meshRenderer = mr;
		scene->Add(std::move(go));
	};
	makeSphere("Metal Sphere", glm::vec3(-4.5f, 0.0f, 0.0f), glm::vec3(0.95f, 0.75f, 0.35f), 1.0f, 0.2f);
	makeSphere("Plastic Sphere", glm::vec3(4.5f, 0.0f, 0.0f), glm::vec3(0.1f, 0.4f, 0.85f), 0.0f, 0.4f);

	// --- Lights as scene objects ---
	auto makeLight = [&](const char* name, LightComponent::Type type, glm::vec3 pos,
		glm::vec3 color, float intensity, glm::vec3 dir)
	{
		auto go = std::make_unique<GameObject>();
		go->name = name;
		go->transform.position = pos;
		LightComponent lc;
		lc.type = type; lc.color = color; lc.intensity = intensity; lc.direction = dir;
		go->light = lc;
		scene->Add(std::move(go));
	};
	makeLight("Sun (Directional)", LightComponent::Type::Directional, glm::vec3(0.0f, 10.0f, 0.0f),
		glm::vec3(1.0f, 0.97f, 0.9f), 2.5f, glm::normalize(glm::vec3(-0.4f, -1.0f, -0.5f)));
	makeLight("Point Light A", LightComponent::Type::Point, glm::vec3(-8.0f, 6.0f, 8.0f),
		glm::vec3(1.0f, 0.4f, 0.3f), 300.0f, glm::vec3(0, -1, 0));
	makeLight("Point Light B", LightComponent::Type::Point, glm::vec3(8.0f, 6.0f, 8.0f),
		glm::vec3(0.3f, 0.5f, 1.0f), 300.0f, glm::vec3(0, -1, 0));
	makeLight("Spot Light", LightComponent::Type::Spot, glm::vec3(0.0f, 2.0f, 12.0f),
		glm::vec3(1.0f), 500.0f, glm::vec3(0.0f, 0.0f, -1.0f));

	scene->Select(0);
}

LightSet Application::GatherLights()
{
	LightSet ls;
	ls.hasDirectional = false;
	for (auto& go : scene->Objects())
	{
		if (!go->active || !go->light || !go->light->enabled) continue;
		const LightComponent& l = *go->light;
		switch (l.type)
		{
		case LightComponent::Type::Directional:
			ls.hasDirectional = true;
			ls.directional.direction = l.direction;
			ls.directional.color = l.color;
			ls.directional.intensity = l.intensity;
			break;
		case LightComponent::Type::Point:
			ls.points.push_back({ go->transform.position, l.color, l.intensity });
			break;
		case LightComponent::Type::Spot:
		{
			SpotLight s;
			s.position = go->transform.position;
			s.direction = l.direction;
			s.color = l.color;
			s.intensity = l.intensity;
			s.innerCutoffDeg = l.innerCutoffDeg;
			s.outerCutoffDeg = l.outerCutoffDeg;
			ls.spots.push_back(s);
			break;
		}
		}
	}
	return ls;
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

	LightSet ls = GatherLights();
	ls.Upload(*pbrShader);

	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_useIBL"), settings.useIBL ? 1 : 0);
	glUniform1f(glGetUniformLocation(pbrShader->ID, "u_maxReflectionLod"), ibl->MaxReflectionLod());
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_CUBE_MAP, ibl->IrradianceMap());
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_CUBE_MAP, ibl->PrefilterMap());
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, ibl->BrdfLUT());

	for (auto& go : scene->Objects())
	{
		if (!go->active || !go->meshRenderer || !go->meshRenderer->enabled) continue;
		MeshRenderer& mr = *go->meshRenderer;
		mr.material.Bind(*pbrShader);
		glm::mat4 modelMat = go->transform.Matrix();
		pbrShader->SetUniformMat4("model", modelMat);
		if (mr.kind == MeshRenderer::Geometry::Model && mr.model)
			mr.model->DrawGeometry();
		else if (mr.mesh)
			mr.mesh->Draw();
	}

	if (settings.showSkybox)
		ibl->RenderBackground(
			camera->GetProjectionMatrix(fov, 0.1f, 200.0f),
			camera->GetViewMatrix(),
			settings.skyboxIntensity);

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
	glUniform1f(glGetUniformLocation(hdrShader->ID, "u_exposure"), settings.exposure);
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
		window.PollEvents();

		double now = glfwGetTime();
		Time::_DeltaTime = (float)(now - lastFrame);
		lastFrame = now;
		frameCount++;
		if (now - prevFpsTime >= 0.5)
		{
			settings.fps = (float)(frameCount / (now - prevFpsTime));
			std::string title = "OpenGLCore - PBR Editor - " + std::to_string((int)settings.fps) + " FPS";
			window.SetTitle(title.c_str());
			hudText = "FPS: " + std::to_string((int)settings.fps);
			prevFpsTime = now;
			frameCount = 0;
		}

		// Build the editor UI first so capture flags reflect this frame's interaction.
		editor.BeginFrame();
		editor.BuildUI(*scene, settings);

		// Camera only responds when ImGui is not using the mouse/keyboard.
		if (!editor.WantCaptureMouse() && !editor.WantCaptureKeyboard())
			camera->Inputs(window.Handle(), Time::_DeltaTime);
		camera->updateMatrix(fov, 0.1f, 200.0f);

		RenderSceneHDR();
		TonemapToScreen();
		hud->RenderText(hudText, 25.0f, (float)fbHeight - 60.0f, 0.6f, glm::vec3(0.9f, 0.9f, 0.2f));

		editor.EndFrame();

		window.SwapBuffers();
	}
}

Application::~Application()
{
	editor.Shutdown();
	if (hdrColor) glDeleteTextures(1, &hdrColor);
	if (hdrDepth) glDeleteRenderbuffers(1, &hdrDepth);
	if (hdrFBO) glDeleteFramebuffers(1, &hdrFBO);
	if (emptyVAO) glDeleteVertexArrays(1, &emptyVAO);
}
