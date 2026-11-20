#include "Application.h"
#include "Geometry.h"
#include "GLUtils.h"
#include "EngineTime.h"
#include "SceneIO.h"
#include "ScriptRegistry.h"
#include <stb/stb_image_write.h>
#include <cstdlib>
#include <iostream>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <vector>
#include <cmath>

namespace ogl {

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
	gridShader = std::make_unique<Shader>("./Shader/grid.vert", "./Shader/grid.frag");
	ibl = std::make_unique<IBL>("./SKYBOX/ballroom_4k.exr");
	hud = std::make_unique<Text>("./Fonts/arial.ttf", 48, fbWidth, fbHeight);

	CreateHdrTarget(fbWidth, fbHeight);
	glGenVertexArrays(1, &emptyVAO);

	assets = std::make_unique<AssetManager>();
	lines = std::make_unique<LineRenderer>();
	physics = std::make_unique<PhysicsScene>();
	input.Init(window.Handle());
	ScriptRegistry::RegisterBuiltins();
	BuildPrimitiveLibrary();

	pbrShader->Activate();
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_irradianceMap"), 0);
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_prefilterMap"), 1);
	glUniform1i(glGetUniformLocation(pbrShader->ID, "u_brdfLUT"), 2);

	scene = std::make_unique<Scene>();
	BuildScene();

	editor.Init(window.Handle(), camera.get(), &primitives, assets.get(), &input, &scripts, physics.get());
	editor.Log(std::string("Physics: ") + (physics->Available() ? "NVIDIA PhysX ready" : "PhysX unavailable"));
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
		// Physics: a dynamic rigid body with a sphere collider (drops onto the ground on Play).
		ColliderComponent co; co.shape = ColliderComponent::Shape::Sphere; co.radius = 1.0f;
		co.material.restitution = 0.4f;
		go->collider = co;
		RigidBodyComponent rb; rb.type = RigidBodyComponent::BodyType::Dynamic; rb.mass = 1.0f;
		go->rigidBody = rb;
		scene->Add(std::move(go));
	};
	// On Play the spheres fall under gravity onto the ground and bounce (PhysX).
	makeSphere("Metal Sphere", glm::vec3(-4.5f, 4.0f, 0.0f), glm::vec3(0.95f, 0.75f, 0.35f), 1.0f, 0.2f, nullptr);
	makeSphere("Plastic Sphere", glm::vec3(4.5f, 6.0f, 0.0f), glm::vec3(0.1f, 0.4f, 0.85f), 0.0f, 0.4f, nullptr);

	// --- Ground plane (static collider) ---
	{
		const PrimitiveDef& planeDef = primitives.Get(PrimitiveType::Plane);
		auto go = std::make_unique<GameObject>();
		go->name = "Ground";
		go->transform.position = glm::vec3(0.0f, -2.0f, 0.0f);
		go->transform.scale = glm::vec3(20.0f, 1.0f, 20.0f);
		MeshRenderer mr;
		mr.kind = MeshRenderer::Geometry::Mesh;
		mr.mesh = planeDef.mesh;
		mr.sourceId = planeDef.name;
		mr.meshName = "Plane";
		mr.materialName = "Ground";
		mr.material.albedo = glm::vec3(0.35f);
		mr.material.roughness = 0.9f;
		mr.localMin = planeDef.localMin;
		mr.localMax = planeDef.localMax;
		go->meshRenderer = mr;
		ColliderComponent co; co.shape = ColliderComponent::Shape::Box;
		co.halfExtents = glm::vec3(1.0f, 0.05f, 1.0f); // * object scale -> 20 x 1 x 20 in PhysX
		go->collider = co; // no rigidBody -> static
		scene->Add(std::move(go));
	}

	// --- Scripted (non-physics) cube: spins on Play to demonstrate scripting alongside physics ---
	{
		const PrimitiveDef& cubeDef = primitives.Get(PrimitiveType::Cube);
		auto go = std::make_unique<GameObject>();
		go->name = "Spinner Cube";
		go->transform.position = glm::vec3(0.0f, 1.0f, -3.0f);
		MeshRenderer mr;
		mr.kind = MeshRenderer::Geometry::Mesh;
		mr.mesh = cubeDef.mesh; mr.sourceId = cubeDef.name; mr.meshName = "Cube"; mr.materialName = "Default";
		mr.material.albedo = glm::vec3(0.6f, 0.2f, 0.7f); mr.material.roughness = 0.4f;
		mr.localMin = cubeDef.localMin; mr.localMax = cubeDef.localMax;
		go->meshRenderer = mr;
		ScriptComponent sc; sc.behaviorName = "Rotator"; go->script = sc;
		scene->Add(std::move(go));
	}

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

	// --- Main game camera (used to render in Play mode; distinct from the editor scene camera) ---
	{
		auto go = std::make_unique<GameObject>();
		go->name = "Main Camera";
		glm::vec3 pos(9.0f, 4.0f, 11.0f);
		go->transform.position = pos;
		glm::vec3 fwd = glm::normalize(glm::vec3(0.0f, 2.0f, 0.0f) - pos); // look at the scene
		go->transform.rotationEuler = glm::vec3(
			glm::degrees(std::asin(glm::clamp(fwd.y, -1.0f, 1.0f))),
			glm::degrees(std::atan2(-fwd.x, -fwd.z)), 0.0f);
		CameraComponent cc; cc.isMain = true; cc.fov = 55.0f;
		go->cameraComp = cc;
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

void Application::UpdateActiveCamera(bool playing)
{
	GameObject* gcam = playing ? scene->ActiveCamera() : nullptr;
	if (gcam)
	{
		glm::mat4 world = scene->WorldMatrix(gcam);
		glm::vec3 pos = glm::vec3(world[3]);
		glm::vec3 fwd = -glm::normalize(glm::vec3(world[2])); // camera looks down local -Z
		glm::vec3 up = glm::normalize(glm::vec3(world[1]));
		const CameraComponent& c = *gcam->cameraComp;
		activeView = glm::lookAt(pos, pos + fwd, up);
		activeProj = glm::perspective(glm::radians(c.fov), (float)fbWidth / (float)fbHeight, c.nearClip, c.farClip);
		activeCamPos = pos;
		usingGameCamera = true;
	}
	else
	{
		camera->updateMatrix(fov, 0.1f, 200.0f);
		activeView = camera->GetViewMatrix();
		activeProj = camera->GetProjectionMatrix(fov, 0.1f, 200.0f);
		activeCamPos = camera->Position;
		usingGameCamera = false;
	}
	activeViewProj = activeProj * activeView;
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
	pbrShader->SetUniformValueVec3("u_camPos", activeCamPos);
	glUniformMatrix4fv(glGetUniformLocation(pbrShader->ID, "camMatrix"), 1, GL_FALSE, glm::value_ptr(activeViewProj));

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

	if (!editor.IsPlaying()) RenderSelectionOutline();

	glDisable(GL_STENCIL_TEST);
	glStencilMask(0xFF); // restore so the next frame's clear works

	// Editor overlays (grid, light gizmos) are a Scene-view aid: hide them in Play mode so the
	// game camera shows a clean "Game view". Physics debug stays controlled by its own toggle.
	if (!editor.IsPlaying())
	{
		RenderInfiniteGrid();
		RenderLightGizmos();
	}
	RenderPhysicsDebug();

	if (settings.showSkybox)
		ibl->RenderBackground(activeProj, activeView, settings.skyboxIntensity);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::RenderLightGizmos()
{
	const glm::mat4 viewProj = activeViewProj; // proj * view (active camera this frame)

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

	// Camera frustum gizmos (white) so game cameras are visible in the Scene view.
	for (auto& go : scene->Objects())
	{
		if (!go->active || !go->cameraComp) continue;
		const CameraComponent& cc = *go->cameraComp;
		glm::mat4 world = scene->WorldMatrix(go.get());
		glm::vec3 pos(world[3]);
		glm::vec3 right = glm::normalize(glm::vec3(world[0]));
		glm::vec3 up = glm::normalize(glm::vec3(world[1]));
		glm::vec3 fwd = -glm::normalize(glm::vec3(world[2]));
		float aspect = (fbHeight > 0) ? (float)fbWidth / (float)fbHeight : 1.7778f;
		float nd = 0.4f, fd = 3.0f;                 // gizmo near/far distances (icon-sized)
		float tv = std::tan(glm::radians(cc.fov) * 0.5f);
		auto rect = [&](float d) {
			float hh = tv * d, hw = hh * aspect;
			glm::vec3 c = pos + fwd * d;
			return std::array<glm::vec3, 4>{ c - right * hw - up * hh, c + right * hw - up * hh,
				c + right * hw + up * hh, c - right * hw + up * hh };
		};
		auto n = rect(nd); auto f = rect(fd);
		for (int i = 0; i < 4; ++i) { lines->AddLine(n[i], n[(i + 1) % 4]); lines->AddLine(f[i], f[(i + 1) % 4]); lines->AddLine(n[i], f[i]); }
		for (int i = 0; i < 4; ++i) lines->AddLine(pos, n[i]); // apex lines
	}
	lines->Flush(viewProj, glm::vec3(0.9f, 0.9f, 0.95f));
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
	glm::mat4 mvp = activeViewProj * scene->WorldMatrix(sel);
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

void Application::RenderInfiniteGrid()
{
	if (!settings.showGrid) return;

	// A procedural, camera-independent ground grid: a full-screen triangle whose fragments
	// reconstruct the world position on the y=0 plane (see grid.vert/frag). Depth is written so
	// scene geometry occludes it; alpha-blended with distance fade to avoid a hard boundary.
	glm::mat4 vp = activeViewProj;
	glm::mat4 inv = glm::inverse(vp);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	gridShader->Activate();
	glUniformMatrix4fv(glGetUniformLocation(gridShader->ID, "u_invViewProj"), 1, GL_FALSE, glm::value_ptr(inv));
	glUniformMatrix4fv(glGetUniformLocation(gridShader->ID, "u_viewProj"), 1, GL_FALSE, glm::value_ptr(vp));
	glUniform3fv(glGetUniformLocation(gridShader->ID, "u_camPos"), 1, glm::value_ptr(activeCamPos));
	float spacing = settings.gridSpacing > 0.0f ? settings.gridSpacing : 1.0f;
	glUniform1f(glGetUniformLocation(gridShader->ID, "u_spacing"), spacing);
	// Fade relative to how high the camera is, so the grid stays stable while flying.
	float h = glm::max(std::abs(activeCamPos.y), 1.0f);
	glUniform1f(glGetUniformLocation(gridShader->ID, "u_fadeStart"), h * 4.0f + 20.0f);
	glUniform1f(glGetUniformLocation(gridShader->ID, "u_fadeEnd"), h * 20.0f + 120.0f);

	glBindVertexArray(emptyVAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);

	glDisable(GL_BLEND);
}

void Application::RenderPhysicsDebug()
{
	if (!settings.showPhysicsDebug) return;
	std::vector<DebugLine> dl;
	PhysicsDebug::BuildColliderLines(*scene, dl);
	if (dl.empty()) return;

	// Group by colour so each flush is one draw (collider green vs trigger blue).
	const glm::mat4 viewProj = activeViewProj;
	auto flushColor = [&](const glm::vec3& c)
	{
		bool any = false;
		for (const DebugLine& l : dl)
			if (l.color == c) { lines->AddLine(l.a, l.b); any = true; }
		if (any) lines->Flush(viewProj, c, 1.5f);
	};
	flushColor(glm::vec3(0.2f, 0.9f, 0.4f)); // colliders
	flushColor(glm::vec3(0.2f, 0.7f, 1.0f)); // triggers
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
			if (g->meshRenderer && g->meshRenderer->model)
				std::cout << "[SELFTEST] model collision verts=" << g->meshRenderer->model->CollisionPositions().size()
					<< " indices=" << g->meshRenderer->model->CollisionIndices().size() << std::endl;
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
				scripts.Begin(reloaded, input, physics.get(), 0.016f);
				for (int i = 0; i < 10; ++i) scripts.Update(reloaded, input, physics.get(), 0.016f);
				scripts.End();
				scriptsOk = (spin->transform.rotationEuler.y != y0);
				std::cout << "[SELFTEST] script Rotator dy=" << (spin->transform.rotationEuler.y - y0)
					<< " running=" << scripts.IsRunning() << std::endl;
			}
			else std::cout << "[SELFTEST] (no Rotator-scripted object found)" << std::endl;
		}

		// Exercise PhysX headlessly: drop a dynamic sphere and confirm it falls and settles.
		bool physicsOk = false;
		if (loaded && physics && physics->Available())
		{
			GameObject* ball = nullptr;
			for (auto& o : reloaded.Objects())
				if (o->rigidBody && o->rigidBody->type == RigidBodyComponent::BodyType::Dynamic) { ball = o.get(); break; }
			if (ball)
			{
				float y0 = ball->transform.position.y;
				physics->Begin(reloaded);
				for (int i = 0; i < 120; ++i) physics->Step(reloaded, 1.0f / 60.0f); // ~2s
				float y1 = ball->transform.position.y;
				physics->End();
				physicsOk = (y1 < y0 - 0.5f) && (y1 > -5.0f); // fell, but landed on the ground
				std::cout << "[SELFTEST] PhysX drop y0=" << y0 << " y1=" << y1
					<< " available=" << physics->Available() << std::endl;
			}
		}
		else std::cout << "[SELFTEST] PhysX unavailable" << std::endl;

		// Exercise convex-mesh cooking: switch a dynamic object's collider to ConvexMesh so the
		// physics layer must cook a hull from the retained renderer geometry, then drop it.
		bool meshColliderOk = false;
		if (loaded && physics && physics->Available())
		{
			GameObject* ball = nullptr;
			for (auto& o : reloaded.Objects())
				if (o->rigidBody && o->rigidBody->type == RigidBodyComponent::BodyType::Dynamic
					&& o->meshRenderer && (o->meshRenderer->mesh || o->meshRenderer->model))
				{ ball = o.get(); break; }
			if (ball && ball->collider)
			{
				ball->collider->shape = ColliderComponent::Shape::ConvexMesh;
				ball->transform.position.y = 4.0f;
				float y0 = ball->transform.position.y;
				physics->Begin(reloaded);
				for (int i = 0; i < 120; ++i) physics->Step(reloaded, 1.0f / 60.0f);
				float y1 = ball->transform.position.y;
				physics->End();
				meshColliderOk = (y1 < y0 - 0.5f) && (y1 > -5.0f);
				std::cout << "[SELFTEST] ConvexMesh cook drop y0=" << y0 << " y1=" << y1 << std::endl;
			}
			else std::cout << "[SELFTEST] (no mesh-renderer dynamic object for cook test)" << std::endl;
		}

		std::cout << "[SELFTEST] "
			<< ((saved && loaded && reloaded.Count() == before && scriptsOk && physicsOk && meshColliderOk) ? "PASS" : "FAIL") << std::endl;
		return;
	}

	// Headless screenshot. OGL_SCREENSHOT=1 renders the editor (scene camera); =play renders
	// through the active game camera (Play mode) to verify the camera switch.
	char ssVal[8] = {}; size_t ssLen = 0; getenv_s(&ssLen, ssVal, sizeof(ssVal), "OGL_SCREENSHOT");
	if (ssLen > 0)
	{
		bool playShot = (std::string(ssVal) == "play");
		if (playShot)
			editor.SetPlayMode(*scene, true);
		else
		{
			camera->Position = glm::vec3(2.2f, 3.4f, 5.0f);
			camera->Orientation = glm::normalize(glm::vec3(-0.35f, -0.18f, -1.0f));
		}
		for (int i = 0; i < 4; ++i)
		{
			if (playShot) editor.Update(*scene, 1.0f / 60.0f); // tick scripts/physics
			UpdateActiveCamera(playShot);
			RenderSceneHDR();
			TonemapToScreen();
			hud->RenderText(playShot ? "game camera" : "scene camera", 25.0f, (float)fbHeight - 60.0f, 0.6f, glm::vec3(0.9f));
			window.SwapBuffers();
			window.PollEvents();
		}
		std::vector<unsigned char> px((size_t)fbWidth * fbHeight * 3);
		glReadPixels(0, 0, fbWidth, fbHeight, GL_RGB, GL_UNSIGNED_BYTE, px.data());
		stbi_flip_vertically_on_write(1);
		stbi_write_png("screenshot.png", fbWidth, fbHeight, 3, px.data(), fbWidth * 3);
		std::cout << "[SCREENSHOT] wrote screenshot.png (" << fbWidth << "x" << fbHeight << ", "
			<< (playShot ? "game camera" : "scene camera") << ")" << std::endl;
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

		const bool playing = editor.IsPlaying();

		// Active camera for the UI (gizmo/picking align with what's rendered): the game camera
		// in Play, the editor scene camera in Edit.
		UpdateActiveCamera(playing);

		// Build the editor UI (handles selection, gizmo, shortcuts, focus) so capture flags
		// reflect this frame's interaction.
		editor.BeginFrame();
		editor.BuildUI(*scene, settings, activeView, activeProj, fbWidth, fbHeight, Time::_DeltaTime);
		editor.Update(*scene, Time::_DeltaTime); // tick scripts/physics while in Play mode

		// Scene-camera flythrough only in Edit mode (Play renders through the game camera) and
		// only when ImGui/gizmo aren't capturing input.
		if (!playing && !editor.WantCaptureMouse() && !editor.WantCaptureKeyboard() && !editor.WantCaptureViewport())
			camera->Inputs(window.Handle(), Time::_DeltaTime);

		// Recompute after scripts/physics (and camera input) moved things, for the render pass.
		UpdateActiveCamera(playing);

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

} // namespace ogl