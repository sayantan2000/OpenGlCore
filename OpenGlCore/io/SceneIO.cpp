#include "SceneIO.h"
#include "Scene.h"
#include "GameObject.h"
#include "AssetManager.h"
#include "PrimitiveLibrary.h"
#include "Model.h"

#include <json/json.hpp>
#include <fstream>
#include <sstream>
#include <memory>

namespace ogl {

using json = nlohmann::json;

namespace
{
	json Vec3(const glm::vec3& v) { return json::array({ v.x, v.y, v.z }); }

	glm::vec3 ToVec3(const json& j, const glm::vec3& def = glm::vec3(0.0f))
	{
		if (!j.is_array() || j.size() != 3) return def;
		return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
	}

	json MaterialToJson(const PbrMaterial& m)
	{
		return json{
			{"albedo", Vec3(m.albedo)}, {"metallic", m.metallic}, {"roughness", m.roughness}, {"ao", m.ao},
			{"albedoMap", m.albedoMapPath}, {"normalMap", m.normalMapPath}, {"metallicMap", m.metallicMapPath},
			{"roughnessMap", m.roughnessMapPath}, {"aoMap", m.aoMapPath},
		};
	}

	// Serialize the whole scene to a JSON object.
	json SceneToJson(Scene& scene)
	{
		json root;
		root["version"] = 2;
		json arr = json::array();
		for (auto& up : scene.Objects())
		{
			const GameObject* go = up.get();
			json j;
			j["id"] = go->id;
			j["parentId"] = go->parentId;
			j["name"] = go->name;
			j["active"] = go->active;
			j["transform"] = {
				{"position", Vec3(go->transform.position)},
				{"rotation", Vec3(go->transform.rotationEuler)},
				{"scale", Vec3(go->transform.scale)},
			};
			if (go->meshRenderer)
			{
				const MeshRenderer& mr = *go->meshRenderer;
				j["meshRenderer"] = {
					{"kind", mr.kind == MeshRenderer::Geometry::Model ? "model" : "primitive"},
					{"source", mr.sourceId}, {"meshName", mr.meshName}, {"materialName", mr.materialName},
					{"enabled", mr.enabled}, {"localMin", Vec3(mr.localMin)}, {"localMax", Vec3(mr.localMax)},
					{"material", MaterialToJson(mr.material)},
				};
			}
			if (go->light)
			{
				const LightComponent& l = *go->light;
				j["light"] = {
					{"type", (int)l.type}, {"color", Vec3(l.color)}, {"intensity", l.intensity},
					{"direction", Vec3(l.direction)}, {"innerCutoff", l.innerCutoffDeg},
					{"outerCutoff", l.outerCutoffDeg}, {"width", l.width}, {"height", l.height},
					{"enabled", l.enabled},
				};
			}
			if (go->script)
			{
				json params = json::object();
				for (const auto& [k, v] : go->script->params) params[k] = v;
				j["script"] = { {"behavior", go->script->behaviorName}, {"enabled", go->script->enabled}, {"params", params} };
			}
			if (go->rigidBody)
			{
				const RigidBodyComponent& rb = *go->rigidBody;
				j["rigidBody"] = {
					{"type", (int)rb.type}, {"mass", rb.mass}, {"useGravity", rb.useGravity},
					{"linearDamping", rb.linearDamping}, {"angularDamping", rb.angularDamping},
					{"initLinVel", Vec3(rb.initialLinearVelocity)}, {"initAngVel", Vec3(rb.initialAngularVelocity)},
					{"freezePos", json::array({rb.freezePosX, rb.freezePosY, rb.freezePosZ})},
					{"freezeRot", json::array({rb.freezeRotX, rb.freezeRotY, rb.freezeRotZ})},
					{"enabled", rb.enabled},
				};
			}
			if (go->collider)
			{
				const ColliderComponent& co = *go->collider;
				j["collider"] = {
					{"shape", (int)co.shape}, {"center", Vec3(co.center)}, {"halfExtents", Vec3(co.halfExtents)},
					{"radius", co.radius}, {"halfHeight", co.halfHeight}, {"isTrigger", co.isTrigger},
					{"material", { {"staticFriction", co.material.staticFriction}, {"dynamicFriction", co.material.dynamicFriction}, {"restitution", co.material.restitution} }},
					{"enabled", co.enabled},
				};
			}
			arr.push_back(std::move(j));
		}
		root["objects"] = std::move(arr);
		return root;
	}

	// Rebuild the scene from a parsed JSON object. Returns false on structural error.
	bool JsonToScene(Scene& scene, const json& root, AssetManager& assets,
		const PrimitiveLibrary& primitives, std::string* error)
	{
		if (!root.contains("objects") || !root["objects"].is_array())
		{
			if (error) *error = "missing 'objects' array";
			return false;
		}

		scene.Clear();
		for (const json& j : root["objects"])
		{
			auto go = std::make_unique<GameObject>();
			go->id = j.value("id", 0u);
			go->parentId = j.value("parentId", 0u);
			go->name = j.value("name", std::string("GameObject"));
			go->active = j.value("active", true);

			if (j.contains("transform"))
			{
				const json& t = j["transform"];
				go->transform.position = ToVec3(t.value("position", json()), glm::vec3(0.0f));
				go->transform.rotationEuler = ToVec3(t.value("rotation", json()), glm::vec3(0.0f));
				go->transform.scale = ToVec3(t.value("scale", json()), glm::vec3(1.0f));
			}

			if (j.contains("meshRenderer"))
			{
				const json& m = j["meshRenderer"];
				MeshRenderer mr;
				std::string kind = m.value("kind", std::string("primitive"));
				mr.sourceId = m.value("source", std::string());
				mr.meshName = m.value("meshName", std::string("Mesh"));
				mr.materialName = m.value("materialName", std::string("Material"));
				mr.enabled = m.value("enabled", true);
				mr.localMin = ToVec3(m.value("localMin", json()), glm::vec3(-1.0f));
				mr.localMax = ToVec3(m.value("localMax", json()), glm::vec3(1.0f));

				if (kind == "model")
				{
					mr.kind = MeshRenderer::Geometry::Model;
					mr.model = assets.GetModel(mr.sourceId);
					if (mr.model && !m.contains("localMin")) mr.localMin = mr.model->BoundsMin();
					if (mr.model && !m.contains("localMax")) mr.localMax = mr.model->BoundsMax();
				}
				else
				{
					mr.kind = MeshRenderer::Geometry::Mesh;
					if (const PrimitiveDef* def = primitives.FindByName(mr.sourceId))
					{
						mr.mesh = def->mesh;
						if (!m.contains("localMin")) mr.localMin = def->localMin;
						if (!m.contains("localMax")) mr.localMax = def->localMax;
					}
				}

				if (m.contains("material"))
				{
					const json& mat = m["material"];
					PbrMaterial& pm = mr.material;
					pm.albedo = ToVec3(mat.value("albedo", json()), glm::vec3(1.0f));
					pm.metallic = mat.value("metallic", 0.0f);
					pm.roughness = mat.value("roughness", 0.5f);
					pm.ao = mat.value("ao", 1.0f);
					pm.albedoMapPath = mat.value("albedoMap", std::string());
					pm.normalMapPath = mat.value("normalMap", std::string());
					pm.metallicMapPath = mat.value("metallicMap", std::string());
					pm.roughnessMapPath = mat.value("roughnessMap", std::string());
					pm.aoMapPath = mat.value("aoMap", std::string());
					pm.albedoMap = assets.GetTexture(pm.albedoMapPath, true);
					pm.normalMap = assets.GetTexture(pm.normalMapPath);
					pm.metallicMap = assets.GetTexture(pm.metallicMapPath);
					pm.roughnessMap = assets.GetTexture(pm.roughnessMapPath);
					pm.aoMap = assets.GetTexture(pm.aoMapPath);
				}
				go->meshRenderer = mr;
			}

			if (j.contains("light"))
			{
				const json& l = j["light"];
				LightComponent lc;
				lc.type = (LightComponent::Type)l.value("type", 1);
				lc.color = ToVec3(l.value("color", json()), glm::vec3(1.0f));
				lc.intensity = l.value("intensity", 1.0f);
				lc.direction = ToVec3(l.value("direction", json()), glm::vec3(0.0f, -1.0f, 0.0f));
				lc.innerCutoffDeg = l.value("innerCutoff", 15.0f);
				lc.outerCutoffDeg = l.value("outerCutoff", 25.0f);
				lc.width = l.value("width", 2.0f);
				lc.height = l.value("height", 1.0f);
				lc.enabled = l.value("enabled", true);
				go->light = lc;
			}

			if (j.contains("script"))
			{
				const json& s = j["script"];
				ScriptComponent sc;
				sc.behaviorName = s.value("behavior", std::string());
				sc.enabled = s.value("enabled", true);
				if (s.contains("params") && s["params"].is_object())
					for (auto it = s["params"].begin(); it != s["params"].end(); ++it)
						sc.params[it.key()] = it.value().get<float>();
				go->script = sc;
			}

			if (j.contains("rigidBody"))
			{
				const json& r = j["rigidBody"];
				RigidBodyComponent rb;
				rb.type = (RigidBodyComponent::BodyType)r.value("type", 1);
				rb.mass = r.value("mass", 1.0f);
				rb.useGravity = r.value("useGravity", true);
				rb.linearDamping = r.value("linearDamping", 0.05f);
				rb.angularDamping = r.value("angularDamping", 0.05f);
				rb.initialLinearVelocity = ToVec3(r.value("initLinVel", json()), glm::vec3(0.0f));
				rb.initialAngularVelocity = ToVec3(r.value("initAngVel", json()), glm::vec3(0.0f));
				if (r.contains("freezePos") && r["freezePos"].is_array() && r["freezePos"].size() == 3)
				{ rb.freezePosX = r["freezePos"][0]; rb.freezePosY = r["freezePos"][1]; rb.freezePosZ = r["freezePos"][2]; }
				if (r.contains("freezeRot") && r["freezeRot"].is_array() && r["freezeRot"].size() == 3)
				{ rb.freezeRotX = r["freezeRot"][0]; rb.freezeRotY = r["freezeRot"][1]; rb.freezeRotZ = r["freezeRot"][2]; }
				rb.enabled = r.value("enabled", true);
				go->rigidBody = rb;
			}

			if (j.contains("collider"))
			{
				const json& c = j["collider"];
				ColliderComponent co;
				co.shape = (ColliderComponent::Shape)c.value("shape", 0);
				co.center = ToVec3(c.value("center", json()), glm::vec3(0.0f));
				co.halfExtents = ToVec3(c.value("halfExtents", json()), glm::vec3(0.5f));
				co.radius = c.value("radius", 0.5f);
				co.halfHeight = c.value("halfHeight", 0.5f);
				co.isTrigger = c.value("isTrigger", false);
				if (c.contains("material"))
				{
					const json& m = c["material"];
					co.material.staticFriction = m.value("staticFriction", 0.5f);
					co.material.dynamicFriction = m.value("dynamicFriction", 0.5f);
					co.material.restitution = m.value("restitution", 0.1f);
				}
				co.enabled = c.value("enabled", true);
				go->collider = co;
			}

			scene.Add(std::move(go)); // preserves the saved id, keeps nextId monotonic
		}
		return true;
	}
}

namespace SceneIO
{
	bool Save(Scene& scene, const std::string& path, std::string* error)
	{
		std::ofstream out(path);
		if (!out) { if (error) *error = "cannot open file for writing: " + path; return false; }
		out << SceneToJson(scene).dump(2);
		return true;
	}

	bool Load(Scene& scene, const std::string& path,
		AssetManager& assets, const PrimitiveLibrary& primitives, std::string* error)
	{
		std::ifstream in(path);
		if (!in) { if (error) *error = "cannot open file: " + path; return false; }
		json root;
		try { in >> root; }
		catch (const std::exception& e) { if (error) *error = std::string("JSON parse error: ") + e.what(); return false; }
		return JsonToScene(scene, root, assets, primitives, error);
	}

	std::string SaveToString(Scene& scene)
	{
		return SceneToJson(scene).dump();
	}

	bool LoadFromString(Scene& scene, const std::string& jsonStr,
		AssetManager& assets, const PrimitiveLibrary& primitives, std::string* error)
	{
		json root;
		try { root = json::parse(jsonStr); }
		catch (const std::exception& e) { if (error) *error = std::string("JSON parse error: ") + e.what(); return false; }
		return JsonToScene(scene, root, assets, primitives, error);
	}
}

} // namespace ogl