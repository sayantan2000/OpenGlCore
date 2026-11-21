#include "MaterialAsset.h"
#include "AssetDatabase.h"
#include "AssetManager.h"

#include <json/json.hpp>
#include <fstream>

using json = nlohmann::json;

namespace ogl {

namespace
{
	json G(const Guid& g) { return g.IsValid() ? json(g.ToString()) : json(); }
	Guid ToGuid(const json& j) { return j.is_string() ? Guid::FromString(j.get<std::string>()) : Guid{}; }
	json V2(const glm::vec2& v) { return json::array({ v.x, v.y }); }
	glm::vec2 ToV2(const json& j, glm::vec2 def) { return (j.is_array() && j.size() == 2) ? glm::vec2(j[0].get<float>(), j[1].get<float>()) : def; }
	json V3(const glm::vec3& v) { return json::array({ v.x, v.y, v.z }); }
	glm::vec3 ToV3(const json& j, glm::vec3 def) { return (j.is_array() && j.size() == 3) ? glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>()) : def; }
}

void MaterialAsset::ResetToDefaults()
{
	albedo = glm::vec3(0.8f); metallic = 0.0f; roughness = 0.5f; ao = 1.0f;
	albedoTex = normalTex = metallicTex = roughnessTex = aoTex = Guid{};
	tiling = glm::vec2(1.0f); offset = glm::vec2(0.0f); doubleSided = false;
	shader = "pbr";
}

bool MaterialAsset::Save(const std::string& absPath) const
{
	json j;
	j["kind"] = "Material";
	j["version"] = 1;
	j["guid"] = guid.IsValid() ? guid.ToString() : Guid::Generate().ToString();
	j["name"] = name;
	j["shader"] = shader;
	j["albedo"] = V3(albedo);
	j["metallic"] = metallic;
	j["roughness"] = roughness;
	j["ao"] = ao;
	j["albedoTex"] = G(albedoTex);
	j["normalTex"] = G(normalTex);
	j["metallicTex"] = G(metallicTex);
	j["roughnessTex"] = G(roughnessTex);
	j["aoTex"] = G(aoTex);
	j["tiling"] = V2(tiling);
	j["offset"] = V2(offset);
	j["doubleSided"] = doubleSided;

	std::ofstream out(absPath, std::ios::trunc);
	if (!out) return false;
	out << j.dump(2);
	return true;
}

bool MaterialAsset::Load(const std::string& absPath, MaterialAsset& out)
{
	std::ifstream in(absPath);
	if (!in) return false;
	json j;
	try { in >> j; } catch (...) { return false; }

	out.guid = ToGuid(j.value("guid", json()));
	out.name = j.value("name", std::string("Material"));
	out.shader = j.value("shader", std::string("pbr"));
	out.albedo = ToV3(j.value("albedo", json()), glm::vec3(0.8f));
	out.metallic = j.value("metallic", 0.0f);
	out.roughness = j.value("roughness", 0.5f);
	out.ao = j.value("ao", 1.0f);
	out.albedoTex = ToGuid(j.value("albedoTex", json()));
	out.normalTex = ToGuid(j.value("normalTex", json()));
	out.metallicTex = ToGuid(j.value("metallicTex", json()));
	out.roughnessTex = ToGuid(j.value("roughnessTex", json()));
	out.aoTex = ToGuid(j.value("aoTex", json()));
	out.tiling = ToV2(j.value("tiling", json()), glm::vec2(1.0f));
	out.offset = ToV2(j.value("offset", json()), glm::vec2(0.0f));
	out.doubleSided = j.value("doubleSided", false);
	return true;
}

PbrMaterial MaterialAsset::Resolve(const AssetDatabase& db, AssetManager& assets) const
{
	PbrMaterial m;
	m.albedo = albedo;
	m.metallic = metallic;
	m.roughness = roughness;
	m.ao = ao;

	auto resolveTex = [&](const Guid& g, GLuint& handle, std::string& path, bool srgb)
	{
		if (!g.IsValid()) { handle = 0; path.clear(); return; }
		std::string abs = db.GuidToAbsolutePath(g);
		if (abs.empty()) { handle = 0; path.clear(); return; }
		handle = assets.GetTexture(abs, srgb);
		path = abs;
	};
	resolveTex(albedoTex, m.albedoMap, m.albedoMapPath, true);
	resolveTex(normalTex, m.normalMap, m.normalMapPath, false);
	resolveTex(metallicTex, m.metallicMap, m.metallicMapPath, false);
	resolveTex(roughnessTex, m.roughnessMap, m.roughnessMapPath, false);
	resolveTex(aoTex, m.aoMap, m.aoMapPath, false);
	return m;
}

void MaterialAsset::CaptureFrom(const PbrMaterial& mat, const AssetDatabase& db)
{
	albedo = mat.albedo; metallic = mat.metallic; roughness = mat.roughness; ao = mat.ao;
	// Map absolute texture paths back to GUIDs when they live under the project.
	auto pathToGuid = [&](const std::string& abs) -> Guid
	{
		if (abs.empty()) return Guid{};
		std::string assets = db.AssetsDir();
		std::string norm = abs; for (char& c : norm) if (c == '\\') c = '/';
		if (norm.rfind(assets + "/", 0) == 0)
			return db.PathToGuid(norm.substr(assets.size() + 1));
		return Guid{};
	};
	albedoTex = pathToGuid(mat.albedoMapPath);
	normalTex = pathToGuid(mat.normalMapPath);
	metallicTex = pathToGuid(mat.metallicMapPath);
	roughnessTex = pathToGuid(mat.roughnessMapPath);
	aoTex = pathToGuid(mat.aoMapPath);
}


} // namespace ogl
