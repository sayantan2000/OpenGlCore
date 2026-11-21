#include "AssetDatabase.h"

#include <json/json.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace ogl {

const char* AssetTypeName(AssetType t)
{
	switch (t)
	{
	case AssetType::Folder:   return "Folder";
	case AssetType::Texture:  return "Texture";
	case AssetType::Model:    return "Model";
	case AssetType::Shader:   return "Shader";
	case AssetType::Audio:    return "Audio";
	case AssetType::Material: return "Material";
	case AssetType::Scene:    return "Scene";
	case AssetType::Prefab:   return "Prefab";
	case AssetType::Script:   return "Script";
	default:                  return "Unknown";
	}
}

AssetType AssetTypeFromExtension(const std::string& extIn)
{
	std::string e = extIn;
	std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	if (e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".tga" || e == ".bmp" || e == ".hdr" || e == ".exr") return AssetType::Texture;
	if (e == ".obj" || e == ".fbx" || e == ".gltf" || e == ".glb" || e == ".dae" || e == ".ply") return AssetType::Model;
	if (e == ".glsl" || e == ".vert" || e == ".frag" || e == ".geom" || e == ".comp") return AssetType::Shader;
	if (e == ".wav" || e == ".ogg" || e == ".mp3" || e == ".flac") return AssetType::Audio;
	if (e == ".mat")    return AssetType::Material;
	if (e == ".scene" || e == ".json") return AssetType::Scene;
	if (e == ".prefab") return AssetType::Prefab;
	if (e == ".cpp" || e == ".h" || e == ".hpp" || e == ".lua") return AssetType::Script;
	return AssetType::Unknown;
}

namespace
{
	// Normalize to '/'-separated and strip any leading "./".
	std::string Norm(std::string p)
	{
		std::replace(p.begin(), p.end(), '\\', '/');
		if (p.rfind("./", 0) == 0) p = p.substr(2);
		return p;
	}

	std::int64_t MTime(const fs::path& p)
	{
		std::error_code ec;
		auto t = fs::last_write_time(p, ec);
		if (ec) return 0;
		return (std::int64_t)t.time_since_epoch().count();
	}
}

bool AssetDatabase::CreateProjectSkeleton(const std::string& projectRoot, std::string* err)
{
	static const char* dirs[] = {
		"Assets", "Assets/Scenes", "Assets/Materials", "Assets/Textures", "Assets/Models",
		"Assets/Shaders", "Assets/Scripts", "Assets/Audio", "Assets/Prefabs", "Assets/Plugins",
		"Library", "Temp", "Build", "ProjectSettings"
	};
	std::error_code ec;
	for (const char* d : dirs)
	{
		fs::create_directories(fs::path(projectRoot) / d, ec);
		if (ec) { if (err) *err = "failed to create " + std::string(d) + ": " + ec.message(); return false; }
	}
	return true;
}

bool AssetDatabase::Open(const std::string& projectRoot)
{
	root.clear();
	records.clear();
	byGuid.clear();
	byPath.clear();

	std::string err;
	if (!CreateProjectSkeleton(projectRoot, &err))
	{
		std::cerr << "[AssetDB] " << err << std::endl;
		return false;
	}
	root = Norm(projectRoot);
	if (!root.empty() && root.back() == '/') root.pop_back();
	Refresh();
	return true;
}

std::string AssetDatabase::AssetsDir() const { return root + "/Assets"; }
std::string AssetDatabase::AbsolutePath(const std::string& relPath) const { return AssetsDir() + "/" + Norm(relPath); }

Guid AssetDatabase::EnsureMeta(const std::string& absFile, AssetType type, std::int64_t mtime)
{
	const std::string metaPath = MetaPathFor(absFile);
	Guid guid;

	// Read an existing .meta (keeps the GUID stable across runs and moves).
	std::ifstream in(metaPath);
	if (in)
	{
		try
		{
			json j; in >> j;
			guid = Guid::FromString(j.value("guid", std::string()));
		}
		catch (...) { guid = Guid{}; }
	}
	in.close();

	if (!guid.IsValid())
	{
		guid = Guid::Generate();
		json j;
		j["guid"] = guid.ToString();
		j["type"] = AssetTypeName(type);
		j["importSettings"] = json::object(); // per-type import settings live here
		std::ofstream out(metaPath, std::ios::trunc);
		if (out) out << j.dump(2);
	}
	return guid;
}

void AssetDatabase::ScanDir(const std::string& absDir)
{
	std::error_code ec;
	for (auto it = fs::recursive_directory_iterator(absDir, ec);
		!ec && it != fs::recursive_directory_iterator(); it.increment(ec))
	{
		const fs::directory_entry& entry = *it;
		if (!entry.is_regular_file(ec)) continue;
		fs::path p = entry.path();
		std::string ext = p.extension().string();
		if (ext == ".meta") continue; // sidecars are not assets themselves

		std::string abs = Norm(p.string());
		std::string rel = Norm(fs::relative(p, absDir, ec).string());
		if (ec || rel.empty()) continue;

		AssetType type = AssetTypeFromExtension(ext);
		std::int64_t mt = MTime(p);
		Guid guid = EnsureMeta(abs, type, mt);
		if (!guid.IsValid()) continue;

		AssetRecord rec;
		rec.guid = guid;
		rec.path = rel;
		rec.type = type;
		rec.mtime = mt;
		rec.missing = false;

		size_t idx = records.size();
		records.push_back(std::move(rec));
		byGuid[records[idx].guid] = idx;
		byPath[records[idx].path] = idx;
	}
}

int AssetDatabase::Refresh()
{
	records.clear();
	byGuid.clear();
	byPath.clear();
	if (root.empty()) return 0;
	ScanDir(AssetsDir());
	return (int)records.size();
}

Guid AssetDatabase::RegisterNewAsset(const std::string& relPath)
{
	std::string rel = Norm(relPath);
	if (auto* r = FindByPath(rel)) return r->guid;

	std::string abs = AbsolutePath(rel);
	fs::path p(abs);
	AssetType type = AssetTypeFromExtension(p.extension().string());
	Guid guid = EnsureMeta(abs, type, MTime(p));
	if (!guid.IsValid()) return guid;

	AssetRecord rec;
	rec.guid = guid; rec.path = rel; rec.type = type; rec.mtime = MTime(p); rec.missing = false;
	size_t idx = records.size();
	records.push_back(std::move(rec));
	byGuid[records[idx].guid] = idx;
	byPath[records[idx].path] = idx;
	return guid;
}

const AssetRecord* AssetDatabase::Find(const Guid& guid) const
{
	auto it = byGuid.find(guid);
	return it != byGuid.end() ? &records[it->second] : nullptr;
}
const AssetRecord* AssetDatabase::FindByPath(const std::string& relPath) const
{
	auto it = byPath.find(Norm(relPath));
	return it != byPath.end() ? &records[it->second] : nullptr;
}
Guid AssetDatabase::PathToGuid(const std::string& relPath) const
{
	auto* r = FindByPath(relPath);
	return r ? r->guid : Guid{};
}
std::string AssetDatabase::GuidToPath(const Guid& guid) const
{
	auto* r = Find(guid);
	return r ? r->path : std::string();
}
std::string AssetDatabase::GuidToAbsolutePath(const Guid& guid) const
{
	auto* r = Find(guid);
	return r ? AbsolutePath(r->path) : std::string();
}

std::vector<const AssetRecord*> AssetDatabase::InFolder(const std::string& relFolderIn) const
{
	std::string relFolder = Norm(relFolderIn);
	if (!relFolder.empty() && relFolder.back() == '/') relFolder.pop_back();
	std::vector<const AssetRecord*> out;
	for (const AssetRecord& r : records)
	{
		std::string dir;
		size_t slash = r.path.find_last_of('/');
		if (slash != std::string::npos) dir = r.path.substr(0, slash);
		if (dir == relFolder) out.push_back(&r);
	}
	return out;
}


} // namespace ogl
