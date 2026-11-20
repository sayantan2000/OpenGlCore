#include "Application.h"
#include "Geometry.h"
#include "GLUtils.h"
#include "Time.h"
#include "SceneIO.h"
#include "ScriptRegistry.h"
#include <cstdlib>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <vector>
#include <cmath>

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
	outlineShader = std::make_unique<Shader>("./Shader/outline.vert", "./Shader/outline.frag");
	ibl = std::make_unique<IBL>("./SKYBOX/ballroom_4k.exr");
	hud = std::make_unique<Text>("./Fonts/arial.ttf", 48, fbWidth, fbHeight);

	CreateHdrTarget(fbWidth, fbHeight);
	glGenVertexArrays(1, &emptyVAO);

	assets = std::make_unique<AssetManager>();
	lines = std::make_unique<LineRenderer>();
	input.Init(window.Handle());
	ScriptRegistry::RegisterBuiltins();
	BuildPrimitiveLibrary();

	pbrShader->Activate();
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_irradianceMap"), 0);
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_prefilterMap"), 1);
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_brdfLUT"), 2);

	scene = std::make_unique<Scene>();
	BuildScene();

	editor.Init(window.Handle(), camera.get(), &primitives, assets.get(), &input, &scripts);
	editor.Log(std::string("IBL source: ") + (ibl->UsingHDRI() ? "HDRI (ballroom_4k.exr)" : "procedural sky"));
	editor.Log("Scene loaded: " + std::to_string(scene->Count()) + " objects.");

	glCheckError();
}

void Application::BuildScene()
{
	// --- Wooden chair (Model + full PBR map set, all loaded via the AssetManager) ---
	{
		const std::string dir = "./Models/wooden_chair_ukipagtbw_high/";
		const std::string base = dir + "Wooden_Chair_ukipagtbw_High_4K_";
		const std::string modelPath = dir + "Wooden_Chair_ukipagtbw_High.fbx";
		Model* chair = assets->GetModel(modelPath);

		auto go = std::make_unique<GameObject>();
		go->name = "Wooden Chair";
		MeshRenderer mr;
		mr.kind = MeshRenderer::Geometry::Model;
		mr.model = chair;
		mr.sourceId = modelPath;
		mr.meshName = "Wooden_Chair_ukipagtbw_High.fbx";
		mr.materialName = "Wood (PBR maps)";
		mr.material.albedo = glm::vec3(1.0f);
		mr.material.metallic = 0.0f;
		mr.material.roughness = 1.0f;
		mr.material.ao = 1.0f;
		mr.material.albedoMapPath = base + "BaseColor.jpg";
		mr.material.normalMapPath = base + "Normal.jpg";
		mr.material.roughnessMapPath = base + "Roughness.jpg";
		mr.material.aoMapPath = base + "AO.jpg";
		mr.material.albedoMap = assets->GetTexture(mr.material.albedoMapPath, true);
		mr.material.normalMap = assets->GetTexture(mr.material.normalMapPath);
		mr.material.roughnessMap = assets->GetTexture(mr.material.roughnessMapPath);
		mr.material.aoMap = assets->GetTexture(mr.material.aoMapPath);
		if (chair)
		{
			float fit = chair->ScaleToFit(6.0f);
			go->transform.scale = glm::vec3(fit);
			go->transform.position = -fit * chair->Center(); // reproduce the centered fit as a TRS
			mr.localMin = chair->BoundsMin();
			mr.localMax = chair->BoundsMax();
		}
		go->meshRenderer = mr;
		scene->Add(std::move(go));
	}

	// --- Two spheres with scalar materials (metal + dielectric), using the primitive mesh ---
	const PrimitiveDef& sphereDef = primitives.Get(PrimitiveType::Sphere);
	auto makeSphere = [&](const char* name, glm::vec3 pos, glm::vec3 color, float metal, float rough,
		const char* scriptName)
	{
		auto go = std::make_unique<GameObject>();
		go->name = name;
		go->transform.position = pos;
		go->transform.scale = glm::vec3(1.5f);
		MeshRenderer mr;
		mr.kind = MeshRenderer::Geometry::Mesh;
		mr.mesh = sphereDef.mesh;
		mr.sourceId = sphereDef.name; // "Sphere"
		mr.meshName = "Sphere";
		mr.materialName = name;
		mr.material.albedo = color;
		mr.material.metallic = metal;
		mr.material.roughness = rough;
		mr.material.ao = 1.0f;
		mr.localMin = sphereDef.localMin;
		mr.localMax = sphereDef.localMax;
		go->meshRenderer = mr;
		if (scriptName) { ScriptComponent sc; sc.behaviorName = scriptName; go->script = sc; }
		scene->Add(std::move(go));
	};
	// Scripts demonstrate Play mode: one spins, one bobs (press Play; Stop restores state).
	makeSphere("Metal Sphere", glm::vec3(-4.5f, 0.0f, 0.0f), glm::vec3(0.95f, 0.75f, 0.35f), 1.0f, 0.2f, "Rotator");
	makeSphere("Plastic Sphere", glm::vec3(4.5f, 0.0f, 0.0f), glm::vec3(0.1f, 0.4f, 0.85f), 0.0f, 0.4f, "FloatBob");

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

	// A rectangle (area) light demonstrating the area-light path.
	{
		auto go = std::make_unique<GameObject>();
		go->name = "Area Light";
		go->transform.position = glm::vec3(0.0f, 5.0f, -6.0f);
		LightComponent lc;
		lc.type = LightComponent::Type::Rect;
		lc.color = glm::vec3(1.0f, 0.95f, 0.85f);
		lc.intensity = 25.0f;
		lc.direction = glm::normalize(glm::vec3(0.0f, -0.3f, 1.0f)); // emits toward the scene
		lc.width = 6.0f; lc.height = 3.0f;
		go->light = lc;
		scene->Add(std::move(go));
	}

	scene->Select(0);
}

void Application::BuildPrimitiveLibrary()
{
	const std::vector<VertexAttribute> layout{ {3, GL_FLOAT}, {3, GL_FLOAT}, {2, GL_FLOAT}, {3, GL_FLOAT} };
	auto make = [&](PrimitiveType type, const char* name, glm::vec3 mn, glm::vec3 mx,
		std::vector<GLfloat>& v, std::vector<GLuint>& i)
	{
		primitiveMeshes.push_back(std::make_unique<Mesh>(v, i, layout));
		PrimitiveDef& d = primitives.Get(type);
		d.mesh = primitiveMeshes.back().get();
		d.localMin = mn; d.localMax = mx; d.name = name;
	};

	std::vector<GLfloat> v; std::vector<GLuint> i;
	Geometry::BuildCube(v, i);                 make(PrimitiveType::Cube, "Cube", glm::vec3(-1), glm::vec3(1), v, i);
	Geometry::BuildSphere(v, i, 48, 1.0f);     make(PrimitiveType::Sphere, "Sphere", glm::vec3(-1), glm::vec3(1), v, i);
	Geometry::BuildPlane(v, i, 1);             make(PrimitiveType::Plane, "Plane", glm::vec3(-1, -0.02f, -1), glm::vec3(1, 0.02f, 1), v, i);
	Geometry::BuildCylinder(v, i, 32, 1, 2);   make(PrimitiveType::Cylinder, "Cylinder", glm::vec3(-1), glm::vec3(1), v, i);
	Geometry::BuildCone(v, i, 32, 1, 2);       make(PrimitiveType::Cone, "Cone", glm::vec3(-1), glm::vec3(1), v, i);
	Geometry::BuildTorus(v, i, 48, 24, 0.75f, 0.25f); make(PrimitiveType::Torus, "Torus", glm::vec3(-1, -0.25f, -1), glm::vec3(1, 0.25f, 1), v, i);
	Geometry::BuildCapsule(v, i, 24, 0.5f, 1.0f);     make(PrimitiveType::Capsule, "Capsule", glm::vec3(-0.5f, -1, -0.5f), glm::vec3(0.5f, 1, 0.5f), v, i);
}

LightSet Application::GatherLights()
{
	LightSet ls;
	ls.hasDirectional = false;
	for (auto& go : scene->Objects())
	{
		if (!go->active || !go->light || !go->light->enabled) continue;
		const LightComponent& l = *go->light;
		glm::vec3 worldPos = scene->WorldPosition(go.get());
		switch (l.type)
		{
		case LightComponent::Type::Directional:
			ls.hasDirectional = true;
			ls.directional.direction = l.direction;
			ls.directional.color = l.color;
			ls.directional.intensity = l.intensity;
			break;
		case LightComponent::Type::Point:
			ls.points.push_back({ worldPos, l.color, l.intensity });
			break;
		case LightComponent::Type::Spot:
		{
			SpotLight s;
			s.position = worldPos;
			s.direction = l.direction;
			s.color = l.color;
			s.intensity = l.intensity;
			s.innerCutoffDeg = l.innerCutoffDeg;
			s.outerCutoffDeg = l.outerCutoffDeg;
			ls.spots.push_back(s);
			break;
		}
		case LightComponent::Type::Rect:
		{
			RectLight r;
			r.position = worldPos;
			r.normal = glm::normalize(l.direction);
			// Build an orthonormal basis for the emitter plane.
			glm::vec3 refUp = (std::abs(r.normal.y) > 0.95f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
			r.right = glm::normalize(glm::cross(refUp, r.normal));
			r.up = glm::normalize(glm::cross(r.normal, r.right));
			r.color = l.color;
			r.intensity = l.intensity;
			r.halfWidth = l.width * 0.5f;
			r.halfHeight = l.height * 0.5f;
			ls.rects.push_back(r);
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
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h); // +stencil for selection outline
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, hdrDepth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("HDR framebuffer is not complete");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::DestroyHdrTarget()
{
	if (hdrColor) { glDeleteTextures(1, &hdrColor); hdrColor = 0; }
	if (hdrDepth) { glDeleteRenderbuffers(1, &hdrDepth); hdrDepth = 0; }
	if (hdrFBO) { glDeleteFramebuffers(1, &hdrFBO); hdrFBO = 0; }
}

void Application::ResizeIfNeeded()
{
	int w = 0, h = 0;
	glfwGetFramebufferSize(window.Handle(), &w, &h);
	if (w <= 0 || h <= 0 || (w == fbWidth && h == fbHeight)) return;

	fbWidth = w; fbHeight = h;
	camera->width = (float)w;
	camera->height = (float)h;
	DestroyHdrTarget();
	CreateHdrTarget(w, h);
}

void Application::RenderSceneHDR()
{
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	glViewport(0, 0, fbWidth, fbHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Stencil: write 1 only where the selected object draws (for the outline pass).
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0x00);
	std::uint32_t selId = scene->SelectedId();

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
		glm::mat4 modelMat = scene->WorldMatrix(go.get());
		pbrShader->SetUniformMat4("model", modelMat);
		glStencilMask(go->id == selId ? 0xFF : 0x00); // only the selection writes stencil
		if (mr.kind == MeshRenderer::Geometry::Model && mr.model)
			mr.model->DrawGeometry();
		else if (mr.mesh)
			mr.mesh->Draw();
	}
	glStencilMask(0x00);

	RenderSelectionOutline();

	glDisable(GL_STENCIL_TEST);
	glStencilMask(0xFF); // restore so the next frame's clear works

	RenderLightGizmos();

	if (settings.showSkybox)
		ibl->RenderBackground(
			camera->GetProjectionMatrix(fov, 0.1f, 200.0f),
			camera->GetViewMatrix(),
			settings.skyboxIntensity);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::RenderLightGizmos()
{
	const glm::mat4 viewProj = camera->cameraMatrix; // proj * view (updated this frame)

	// Editor grid floor on the XZ plane (grey lines + coloured X/Z axes).
	{
		const int half = 20;      // grid half-extent in units
		const float e = (float)half;
		for (int i = -half; i <= half; ++i)
		{
			if (i == 0) continue; // the axes are drawn coloured below
			lines->AddLine(glm::vec3(-e, 0.0f, (float)i), glm::vec3(e, 0.0f, (float)i));
			lines->AddLine(glm::vec3((float)i, 0.0f, -e), glm::vec3((float)i, 0.0f, e));
		}
		lines->Flush(viewProj, glm::vec3(0.30f, 0.30f, 0.34f));
		lines->AddLine(glm::vec3(-e, 0.0f, 0.0f), glm::vec3(e, 0.0f, 0.0f));
		lines->Flush(viewProj, glm::vec3(0.65f, 0.25f, 0.25f)); // X axis
		lines->AddLine(glm::vec3(0.0f, 0.0f, -e), glm::vec3(0.0f, 0.0f, e));
		lines->Flush(viewProj, glm::vec3(0.25f, 0.35f, 0.65f)); // Z axis
	}

	// Light wireframes (yellow).
	for (auto& go : scene->Objects())
	{
		if (!go->active || !go->light) continue;
		const LightComponent& l = *go->light;
		glm::vec3 p = scene->WorldPosition(go.get());
		switch (l.type)
		{
		case LightComponent::Type::Point:
			lines->AddCircle(p, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), 0.3f);
			lines->AddCircle(p, glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), 0.3f);
			lines->AddCircle(p, glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), 0.3f);
			break;
		case LightComponent::Type::Directional:
		{
			glm::vec3 d = glm::normalize(l.direction);
			lines->AddLine(p - d * 0.5f, p + d * 1.5f);
			break;
		}
		case LightComponent::Type::Spot:
		{
			glm::vec3 d = glm::normalize(l.direction);
			glm::vec3 end = p + d * 2.0f;
			lines->AddLine(p, end);
			glm::vec3 refUp = (std::abs(d.y) > 0.95f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
			glm::vec3 right = glm::normalize(glm::cross(refUp, d));
			glm::vec3 up = glm::normalize(glm::cross(d, right));
			float r = 2.0f * std::tan(glm::radians(l.outerCutoffDeg));
			lines->AddCircle(end, right, up, r);
			break;
		}
		case LightComponent::Type::Rect:
		{
			glm::vec3 n = glm::normalize(l.direction);
			glm::vec3 refUp = (std::abs(n.y) > 0.95f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
			glm::vec3 right = glm::normalize(glm::cross(refUp, n)) * (l.width * 0.5f);
			glm::vec3 up = glm::normalize(glm::cross(n, glm::normalize(right))) * (l.height * 0.5f);
			glm::vec3 c0 = p - right - up, c1 = p + right - up, c2 = p + right + up, c3 = p - right + up;
			lines->AddLine(c0, c1); lines->AddLine(c1, c2);
			lines->AddLine(c2, c3); lines->AddLine(c3, c0);
			lines->AddLine(p, p + n * 1.0f); // emitter normal
			break;
		}
		}
	}
	lines->Flush(viewProj, glm::vec3(1.0f, 0.85f, 0.2f));
}

void Application::RenderSelectionOutline()
{
	GameObject* sel = scene->SelectedObject();
	if (!sel || !sel->meshRenderer || !sel->meshRenderer->enabled) return;
	MeshRenderer& mr = *sel->meshRenderer;

	// Draw the selected mesh as a slightly inflated shell, only where it didn't write stencil
	// (==1), i.e. the silhouette border. Thickness is object-space, scaled to the mesh size.
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);
	glDisable(GL_DEPTH_TEST);

	outlineShader->Activate();
	glm::mat4 mvp = camera->cameraMatrix * scene->WorldMatrix(sel);
	float localDiag = glm::length(mr.localMax - mr.localMin);
	glUniformMatrix4fv(glGetUniformLocation(outlineShader->ID, "u_mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
	glUniform1f(glGetUniformLocation(outlineShader->ID, "u_thickness"), 0.02f * localDiag);
	glUniform3f(glGetUniformLocation(outlineShader->ID, "u_color"), 1.0f, 0.55f, 0.15f);

	if (mr.kind == MeshRenderer::Geometry::Model && mr.model)
		mr.model->DrawGeometry();
	else if (mr.mesh)
		mr.mesh->Draw();

	glEnable(GL_DEPTH_TEST);
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
	// Headless self-test for scene serialization (no display needed): set OGL_SCENE_SELFTEST
	// to save the built scene, reload it through the AssetManager/PrimitiveLibrary, and report.
	bool selfTest = false;
	{
		size_t len = 0; char buf[8];
		if (getenv_s(&len, buf, sizeof(buf), "OGL_SCENE_SELFTEST") == 0 && len > 0) selfTest = true;
	}
	if (selfTest)
	{
		std::string err;
		size_t before = scene->Count();
		bool saved = SceneIO::Save(*scene, "scene_selftest.json", &err);
		std::cout << "[SELFTEST] save=" << (saved ? "ok" : ("FAIL:" + err))
			<< " objects=" << before << std::endl;
		Scene reloaded;
		bool loaded = SceneIO::Load(reloaded, "scene_selftest.json", *assets, primitives, &err);
		std::cout << "[SELFTEST] load=" << (loaded ? "ok" : ("FAIL:" + err))
			<< " objects=" << reloaded.Count() << std::endl;
		// Spot-check the first object's resolved geometry + a material map.
		if (loaded && reloaded.Count() > 0)
		{
			GameObject* g = reloaded.At(0);
			bool geo = g->meshRenderer && (g->meshRenderer->model || g->meshRenderer->mesh);
			std::cout << "[SELFTEST] first='" << g->name << "' geometryResolved=" << geo
				<< " albedoMap=" << (g->meshRenderer ? g->meshRenderer->material.albedoMap : 0) << std::endl;
		}
		// Exercise the script system (Play-mode logic) headlessly: tick a scripted object and
		// confirm its transform changes, then "stop".
		bool scriptsOk = false;
		if (loaded)
		{
			GameObject* spin = nullptr;
			for (auto& o : reloaded.Objects())
				if (o->script && o->script->behaviorName == "Rotator") { spin = o.get(); break; }
			if (spin)
			{
				float y0 = spin->transform.rotationEuler.y;
				scripts.Begin(reloaded, input, 0.016f);
				for (int i = 0; i < 10; ++i) scripts.Update(reloaded, input, 0.016f);
				scripts.End();
				scriptsOk = (spin->transform.rotationEuler.y != y0);
				std::cout << "[SELFTEST] script Rotator dy=" << (spin->transform.rotationEuler.y - y0)
					<< " running=" << scripts.IsRunning() << std::endl;
			}
			else std::cout << "[SELFTEST] (no Rotator-scripted object found)" << std::endl;
		}

		std::cout << "[SELFTEST] "
			<< ((saved && loaded && reloaded.Count() == before && scriptsOk) ? "PASS" : "FAIL") << std::endl;
		return;
	}

	double lastFrame = glfwGetTime();
	double prevFpsTime = lastFrame;
	int frameCount = 0;

	while (!window.ShouldClose())
	{
		window.PollEvents();
		ResizeIfNeeded();
		input.Update();

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

		// Matrices for this frame's gizmo / picking.
		camera->updateMatrix(fov, 0.1f, 200.0f);
		glm::mat4 view = camera->GetViewMatrix();
		glm::mat4 proj = camera->GetProjectionMatrix(fov, 0.1f, 200.0f);

		// Build the editor UI (handles selection, gizmo, shortcuts, focus) so capture flags
		// reflect this frame's interaction.
		editor.BeginFrame();
		editor.BuildUI(*scene, settings, view, proj, fbWidth, fbHeight, Time::_DeltaTime);
		editor.Update(*scene, Time::_DeltaTime); // tick scripts while in Play mode

		// Camera flythrough only when ImGui/gizmo aren't capturing input.
		if (!editor.WantCaptureMouse() && !editor.WantCaptureKeyboard() && !editor.WantCaptureViewport())
			camera->Inputs(window.Handle(), Time::_DeltaTime);
		camera->updateMatrix(fov, 0.1f, 200.0f); // reflect any movement for the render pass

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
	DestroyHdrTarget();
	if (emptyVAO) glDeleteVertexArrays(1, &emptyVAO);
}
